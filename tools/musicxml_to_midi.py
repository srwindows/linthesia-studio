#!/usr/bin/env python3
"""Linthesia MusicXML -> SMF type 1. Original GPL-compatible implementation.

MusicXML is score data, not an audio format. This importer preserves played
pitches, voices, chords, rests, cross-measure ties, tempo/meter/key and tuplets.
Original input is read-only; an optional .score.tsv preserves engraving marks.
"""
import argparse
from collections import defaultdict
from fractions import Fraction
from pathlib import Path
import re
import struct
import sys
import xml.etree.ElementTree as ET
from zipfile import ZipFile, BadZipFile

PPQ = 960
LIMIT = 16 * 1024 * 1024
MAX_EVENTS = 300_000


def elem(node, name):
    return node.find(name) if node is not None else None


def txt(node, name, default=None):
    child = elem(node, name)
    return default if child is None or child.text is None else child.text.strip()


def integer(node, name, default=0):
    value = txt(node, name, None)
    try:
        return int(value) if value is not None else default
    except (ValueError, OverflowError):
        raise ValueError("MusicXML: número inválido en " + name)


def checked_xml(data, expected="score-partwise"):
    if len(data) > LIMIT:
        raise ValueError("MusicXML demasiado grande")
    if re.search(rb"<!\s*(DOCTYPE|ENTITY)\b", data, re.I):
        raise ValueError("MusicXML con DTD/entidades no admitidas")
    try:
        root = ET.fromstring(data)
    except ET.ParseError as err:
        raise ValueError("MusicXML mal formado: " + str(err))
    if sum(1 for _ in root.iter()) > 400_000:
        raise ValueError("MusicXML excede el límite de elementos")
    for obj in root.iter():
        if isinstance(obj.tag, str) and "}" in obj.tag:
            obj.tag = obj.tag.rsplit("}", 1)[-1]
    accepted=("score-partwise","score-timewise") if expected=="score-partwise" else (expected,)
    if root.tag not in accepted:
        raise ValueError("Documento MusicXML incompatible: se esperaba "+expected)
    return root


def source_bytes(src):
    if src.suffix.lower() != ".mxl":
        return src.read_bytes() if src.stat().st_size <= LIMIT else b"x" * (LIMIT + 1)
    with ZipFile(src) as archive:
        files = {x.filename: x for x in archive.infolist()}
        container = files.get("META-INF/container.xml")
        name = None
        if container and container.file_size < 64_000:
            tree = checked_xml(archive.read(container),"container")
            entry = tree.find(".//rootfile")
            if entry is not None:
                name = entry.attrib.get("full-path")
        if not name:
            candidates = [n for n in files if n.lower().endswith((".musicxml", ".xml"))
                          and not n.startswith("META-INF/")]
            if len(candidates) != 1:
                raise ValueError("Archivo MXL sin partitura principal reconocible")
            name = candidates[0]
        info = files.get(name)
        if not info or info.file_size > LIMIT:
            raise ValueError("Partitura MXL ausente o demasiado grande")
        return archive.read(info)


def integer_vlq(value):
    if value < 0 or value > 0x0fffffff:
        raise ValueError("Duración MIDI fuera de rango")
    out = [value & 127]
    value >>= 7
    while value:
        out.insert(0, (value & 127) | 128)
        value >>= 7
    return bytes(out)


def meta(kind, data):
    return bytes((0xff, kind)) + integer_vlq(len(data)) + data


def chunk(events):
    # tick, priority, payload, deterministic stable ordering.
    events.sort(key=lambda event: (event[0], event[1]))
    out = bytearray()
    current = 0
    for tick, _, data in events:
        if tick < current:
            raise ValueError("Orden de eventos MIDI imposible")
        out.extend(integer_vlq(tick-current))
        out.extend(data)
        current = tick
    out += b"\x00\xff\x2f\x00"
    return b"MTrk" + struct.pack(">I", len(out)) + out


def pitch_of(node, chromatic=0):
    pitch = elem(node, "pitch")
    if pitch is None:
        return None
    step = txt(pitch, "step", "")
    values = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
    if step not in values:
        raise ValueError("Nota MusicXML sin altura musical válida")
    value = (integer(pitch, "octave", 4)+1)*12 + values[step] + \
            integer(pitch, "alter", 0) + chromatic
    if value < 0 or value > 127:
        raise ValueError("Nota MusicXML fuera del rango MIDI 0–127")
    return value


def ticks(duration, divisions):
    if divisions < 1 or divisions > 1_000_000 or duration < 0:
        raise ValueError("Duración o divisiones MusicXML inválidas")
    return int(Fraction(duration * PPQ, divisions) + Fraction(1, 2))


def to_partwise(root):
    if root.tag=="score-partwise":
        return root
    # score-timewise is a transposition of the same open MusicXML standard;
    # preserve measure attributes and each part's musical event order.
    import copy
    dest=ET.Element("score-partwise",root.attrib)
    part_list=root.find("part-list")
    if part_list is None:
        raise ValueError("MusicXML timewise sin part-list")
    dest.append(copy.deepcopy(part_list))
    names=[sp.get("id") for sp in part_list.findall("score-part")]
    if not names or any(not n for n in names):
        raise ValueError("MusicXML timewise sin identificadores de parte")
    parts={name:ET.SubElement(dest,"part",{"id":name}) for name in names}
    for measure in root.findall("measure"):
        for original in measure.findall("part"):
            target=parts.get(original.get("id"))
            if target is None:
                raise ValueError("MusicXML timewise referencia parte desconocida")
            new_measure=ET.SubElement(target,"measure",measure.attrib)
            for child in original:
                new_measure.append(copy.deepcopy(child))
    return dest


def read(root):
    root=to_partwise(root)
    p_list = root.find("part-list")
    programs = {}
    if p_list is not None:
        for sp in p_list.findall("score-part"):
            midi = sp.find("midi-instrument")
            programs[sp.get("id")] = max(0, min(127, integer(midi, "midi-program", 1)-1))
    parts = root.findall("part")
    if not parts:
        raise ValueError("MusicXML sin partes musicales")
    if len(parts) > 15:
        raise ValueError("MusicXML con más de 15 partes: se necesita asignar canales")
    all_tracks = []
    global_meta = [(0, 0, meta(0x51, (500000).to_bytes(3, "big")))]
    annotations = []
    tempos = [(0, 500000)]
    for idx, part in enumerate(parts):
        channel = idx if idx < 9 else idx+1  # channel 10 percussion excluded
        channel &= 0xf
        track = [(0, 0, bytes([0xc0 | channel, programs.get(part.get("id"), 0)]))]
        divisions = 1
        current = 0
        transpose = 0
        tie_open = {}
        last_onset = {}
        measure_start = Fraction(0)
        for measure in part.findall("measure"):
            cursor = Fraction(0)
            end_cursor = Fraction(0)
            last_onset.clear()
            for e in measure:
                if e.tag == "attributes":
                    divisions = integer(e, "divisions", divisions)
                    if not (1 <= divisions <= 1_000_000):
                        raise ValueError("MusicXML divisions fuera de rango")
                    trans = elem(e, "transpose")
                    if trans is not None:
                        transpose = integer(trans, "chromatic", 0) + 12*integer(trans,"octave-change",0)
                    if idx == 0:
                        time = elem(e, "time")
                        if time is not None:
                            num = integer(time, "beats", 4)
                            denom = integer(time, "beat-type", 4)
                            if num < 1 or num > 255 or denom not in (1,2,4,8,16,32,64):
                                raise ValueError("Compás MusicXML no admitido")
                            pow2 = denom.bit_length()-1
                            global_meta.append((int(measure_start), 1, meta(0x58,
                                 bytes((num,pow2,24,8)))))
                        key = elem(e, "key")
                        if key is not None:
                            fifths = integer(key, "fifths", 0)
                            if fifths < -7 or fifths > 7:
                                raise ValueError("Armadura MusicXML inválida")
                            minor = 1 if txt(key,"mode","major")=="minor" else 0
                            global_meta.append((int(measure_start), 1, meta(0x59,
                                 bytes((fifths & 255,minor)))))
                elif e.tag == "direction":
                    sound = elem(e, "sound")
                    bpm = sound.attrib.get("tempo") if sound is not None else None
                    if bpm is None:
                        metronome = e.find("direction-type/metronome")
                        bpm = txt(metronome,"per-minute",None) if metronome is not None else None
                    if bpm is not None:
                        tempo = float(bpm)
                        if tempo < 10 or tempo > 1000 or not (tempo == tempo):
                            raise ValueError("Tempo MusicXML fuera de rango")
                        usec = int(60_000_000 / tempo)
                        tick = int(measure_start + cursor)
                        global_meta.append((tick, 2, meta(0x51,usec.to_bytes(3,"big"))))
                        tempos.append((tick,usec))
                elif e.tag in ("backup","forward"):
                    d = Fraction(integer(e,"duration",0)*PPQ,divisions)
                    cursor += -d if e.tag=="backup" else d
                    if cursor < 0:
                        raise ValueError("MusicXML backup antes del principio del compás")
                    end_cursor = max(end_cursor,cursor)
                elif e.tag == "note":
                    if e.find("grace") is not None:
                        continue
                    voice = txt(e,"voice","1")
                    staff = txt(e,"staff","1")
                    is_chord = elem(e,"chord") is not None
                    length = Fraction(integer(e,"duration",0)*PPQ,divisions)
                    if length < 0:
                        raise ValueError("Duración negativa")
                    onset = last_onset.get((voice,staff),cursor) if is_chord else cursor
                    if not is_chord:
                        last_onset[(voice,staff)] = onset
                    start = int(measure_start+onset+Fraction(1,2))
                    end = int(measure_start+onset+length+Fraction(1,2))
                    if e.find("rest") is None:
                        pitch = pitch_of(e,transpose)
                        if pitch is not None and end > start:
                            notation = e.find("notations")
                            tags = []
                            if notation is not None:
                                arts = notation.find("articulations")
                                if arts is not None:
                                    for a in arts:
                                        if a.tag in ("staccato","staccatissimo","accent",
                                                     "strong-accent","tenuto","detached-legato",
                                                     "marcato"):
                                            tags.append(a.tag)
                                if notation.find("fermata") is not None:
                                    tags.append("fermata")
                                for slur in notation.findall("slur"):
                                    t=slur.get("type","")
                                    if t in ("start","stop"):
                                        tags.append("slur-"+t)
                            key=(voice,staff,pitch)
                            tie_types=[x.get("type","") for x in e.findall("tie")]
                            if "stop" in tie_types and key in tie_open:
                                open_note = tie_open[key]
                                open_note["end"] = end
                                open_note["tags"].update(tags)
                            else:
                                open_note = {"start":start,"end":end,"pitch":pitch,
                                             "channel":channel,"tags":set(tags)}
                            if "start" in tie_types:
                                tie_open[key] = open_note
                            else:
                                tie_open.pop(key,None)
                                track.append((open_note["start"], 3,
                                    bytes([0x90|channel,pitch,88])))
                                track.append((open_note["end"], 2,
                                    bytes([0x80|channel,pitch,0])))
                                annotations.append(open_note)
                    if not is_chord:
                        cursor += length
                    end_cursor=max(end_cursor,cursor,onset+length)
            measure_start += end_cursor
            current = max(current,measure_start)
        # A malformed unfinished tie must not leave a stuck note.
        for note in tie_open.values():
            track.append((note["start"],3,bytes([0x90|channel,note["pitch"],88])))
            track.append((note["end"],2,bytes([0x80|channel,note["pitch"],0])))
            annotations.append(note)
        if len(track) > MAX_EVENTS:
            raise ValueError("Partitura MusicXML demasiado larga")
        all_tracks.append(chunk(track))
    if sum(len(x) for x in all_tracks) > 24*1024*1024:
        raise ValueError("MIDI convertido demasiado grande")
    return (b"MThd"+struct.pack(">IHHH",6,1,len(all_tracks)+1,PPQ)+
            chunk(global_meta)+b"".join(all_tracks),annotations,tempos)


def microsecond_at(tick, tempos):
    tempos = sorted(tempos, key=lambda e:e[0])
    current=0
    usec=0
    tempo=500000
    for at,tempo_new in tempos:
        if at>tick:
            break
        if at>current:
            usec+=(at-current)*tempo//PPQ
        current=at
        tempo=tempo_new
    usec+=(tick-current)*tempo//PPQ
    return usec


def convert(source, output):
    src=Path(source)
    if not src.is_file():
        raise ValueError("MusicXML no encontrado")
    root=checked_xml(source_bytes(src))
    data, notes, tempos=read(root)
    target=Path(output)
    target.parent.mkdir(mode=0o700,parents=True,exist_ok=True)
    temp=target.with_name(target.name+".partial")
    temp.write_bytes(data)
    temp.replace(target)
    sidecar=target.with_suffix(target.suffix+".score.tsv")
    with sidecar.open("w",encoding="utf8") as f:
        f.write("LINTHESIA_MUSICXML_SCORE_1\n")
        for n in notes:
            tags=",".join(sorted(n["tags"]))
            if tags:
                f.write(f'{n["channel"]}\t{n["pitch"]}\t'
                        f'{microsecond_at(n["start"],tempos)}\t{tags}\n')
    return len(notes)


def main(argv=None):
    parser=argparse.ArgumentParser(description="Importar MusicXML/MXL a MIDI abierto para Linthesia")
    parser.add_argument("source")
    parser.add_argument("output")
    args=parser.parse_args(argv)
    try:
        count=convert(args.source,args.output)
        print(f"MusicXML convertido: {count} notas")
        return 0
    except (ValueError, OSError, BadZipFile) as exc:
        print(f"Error de importación MusicXML: {exc}",file=sys.stderr)
        return 2


if __name__=="__main__":
    raise SystemExit(main())
