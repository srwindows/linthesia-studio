from pathlib import Path
import importlib.util
import tempfile
import zipfile
import struct
import sys
import mido
from xml.etree import ElementTree as ET
root=Path(sys.argv[1])
converter=root/'tools/musicxml_to_midi.py'
spec=importlib.util.spec_from_file_location('xml_converter',str(converter))
obj=importlib.util.module_from_spec(spec)
spec.loader.exec_module(obj)
sample='''<?xml version="1.0" encoding="utf-8"?>
<score-partwise version="4.0">
 <part-list><score-part id="P1"><part-name>Piano</part-name><midi-instrument id="I1"><midi-program>1</midi-program></midi-instrument></score-part></part-list>
 <part id="P1">
 <measure number="1">
 <attributes><divisions>10080</divisions><key><fifths>2</fifths><mode>major</mode></key><time><beats>3</beats><beat-type>4</beat-type></time></attributes>
 <direction><sound tempo="90"/></direction>
 <note><pitch><step>C</step><alter>1</alter><octave>5</octave></pitch><duration>3360</duration><voice>1</voice><type>eighth</type><time-modification><actual-notes>3</actual-notes><normal-notes>2</normal-notes></time-modification>
 <notations><articulations><staccato/></articulations><slur type="start" number="1"/></notations></note>
 <note><pitch><step>D</step><octave>5</octave></pitch><duration>3360</duration><voice>1</voice><type>eighth</type></note>
 <note><pitch><step>E</step><octave>5</octave></pitch><duration>3360</duration><voice>1</voice><type>eighth</type>
 <notations><slur type="stop" number="1"/><articulations><accent/></articulations></notations></note>
 <backup><duration>10080</duration></backup>
 <note><pitch><step>C</step><octave>3</octave></pitch><duration>30240</duration><voice>2</voice><type>half</type><dot/></note>
 </measure>
 <measure number="2">
 <note><pitch><step>G</step><octave>4</octave></pitch><duration>10080</duration><tie type="start"/><notations><tied type="start"/></notations></note>
 </measure>
 <measure number="3">
 <note><pitch><step>G</step><octave>4</octave></pitch><duration>10080</duration><tie type="stop"/><notations><tied type="stop"/></notations></note>
 </measure>
 </part></score-partwise>'''
with tempfile.TemporaryDirectory() as temp:
    p=Path(temp)
    song=p/'sample.musicxml';song.write_text(sample)
    mid=p/'sample.mid'
    assert obj.convert(str(song),str(mid))==5
    parsed=mido.MidiFile(mid)
    assert parsed.type==1 and parsed.ticks_per_beat==960
    assert len(parsed.tracks)==2
    msgs=[m for t in parsed.tracks for m in t]
    starts=[m for m in msgs if m.type=='note_on' and m.velocity>0]
    assert len(starts)==5,starts
    assert {x.note for x in starts}=={73,74,76,48,67}
    assert [x for x in msgs if x.type=='time_signature'][0].numerator==3
    assert [x for x in msgs if x.type=='key_signature'][0].key=='D'
    assert [x for x in msgs if x.type=='set_tempo'][-1].tempo==666666
    side=(p/'sample.mid.score.tsv').read_text()
    assert 'staccato' in side and 'slur-start' in side and 'slur-stop' in side
    # Cross-measure tie: one NoteOn + one NoteOff for G4, not retriggered
    assert sum(x.note==67 for x in starts)==1
    assert sum(x.note==67 for x in msgs if x.type=='note_off')==1
    # No DTD or entity expansion allowed
    bad=p/'bad.musicxml';bad.write_text('<!DOCTYPE foo [ <!ENTITY x "hack">]><score-partwise>&x;</score-partwise>')
    try:obj.convert(str(bad),str(p/'bad.mid'));assert False,'DTD accepted'
    except ValueError as err:assert 'DTD' in str(err)
    # ZIP MusicXML (MXL) with namespaced container
    mxl=p/'sample.mxl'
    with zipfile.ZipFile(mxl,'w') as z:
        z.writestr('META-INF/container.xml','''<container xmlns="urn:oasis:names:tc:opendocument:xmlns:container"><rootfiles><rootfile full-path="score.musicxml"/></rootfiles></container>''')
        z.writestr('score.musicxml',sample)
    assert obj.convert(str(mxl),str(p/'mxl.mid'))==5
    assert mido.MidiFile(p/'mxl.mid').ticks_per_beat==960
    # score-timewise equivalent of the partwise fixture.
    source=ET.fromstring(sample)
    timeline=ET.Element("score-timewise")
    timeline.append(source.find("part-list"))
    for m in source.find("part").findall("measure"):
        wrapper=ET.SubElement(timeline,"measure",m.attrib)
        sub=ET.SubElement(wrapper,"part",{"id":"P1"})
        for child in m:
            sub.append(child)
    timewise=p/'timewise.musicxml'
    timewise.write_bytes(ET.tostring(timeline))
    assert obj.convert(str(timewise),str(p/'timewise.mid'))==5
    assert [msg.note for track in mido.MidiFile(p/'timewise.mid').tracks
            for msg in track if msg.type=='note_on' and msg.velocity]>[]
    # Corrupt archives and absence of parts fail safely.
    empty=p/'empty.musicxml';empty.write_text('<score-partwise/>')
    try:obj.convert(str(empty),str(p/'empty.mid'));assert False
    except ValueError as err:assert 'partes' in str(err)
print('OK MusicXML/MXL partwise import: chord/voices/tuplets/tie/tempo/key/time/articulation and hostile input')
