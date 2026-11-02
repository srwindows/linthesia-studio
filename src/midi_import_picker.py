#!/usr/bin/env python3
"""Explorador de archivos para importar MIDI a la carpeta fija 1 de Linthesia.

El explorador solo devuelve rutas absolutas separadas por NUL al confirmar.
La copia de archivos y la proteccion del destino se realizan en Linthesia.
"""
import os
import sys

# Linthesia usa SDL/X11; evita abrir el explorador en otra sesion Wayland.
if os.environ.get("DISPLAY"):
    os.environ["QT_QPA_PLATFORM"] = "xcb"

from PySide6.QtCore import QDir, QSettings, QStandardPaths, QStorageInfo, Qt, QTimer
from PySide6.QtGui import QFont, QKeySequence, QShortcut
from PySide6.QtWidgets import (
    QApplication, QAbstractItemView, QCheckBox, QDialog, QFileSystemModel,
    QHBoxLayout, QLabel, QLineEdit, QListWidget, QListWidgetItem, QMessageBox,
    QInputDialog, QMenu,
    QPushButton, QSplitter, QStyle, QTreeView, QVBoxLayout, QWidget,
)


class Browser(QDialog):
    """Navegacion clasica con historial, lugares y seleccion multiple mixta."""

    def __init__(self):
        super().__init__()
        self.paths = []
        self.settings = QSettings("Linthesia", "ImportadorMIDI")
        self.history = []
        self.history_pos = -1
        self.current_dir = ""
        self.setWindowTitle("Explorador de archivos - Importar MIDI a Añadidos")
        self.resize(1050, 670)
        self.setMinimumSize(740, 440)

        outer = QVBoxLayout(self)
        outer.setContentsMargins(12, 10, 12, 10)
        outer.setSpacing(8)

        title = QLabel("<b>Seleccionar archivos y carpetas para importar</b>")
        outer.addWidget(title)
        info = QLabel("Destino: Añadidos (carpeta fija). Ctrl/Mayus: seleccion multiple; doble clic: abrir carpeta.")
        outer.addWidget(info)

        nav = QHBoxLayout()
        nav.setSpacing(5)
        self.back_button = self._nav_button("Volver atras", QStyle.StandardPixmap.SP_ArrowBack)
        self.next_button = self._nav_button("Ir adelante", QStyle.StandardPixmap.SP_ArrowForward)
        self.up_button = self._nav_button("Subir un nivel", QStyle.StandardPixmap.SP_ArrowUp)
        self.home_button = self._nav_button("Carpeta personal", QStyle.StandardPixmap.SP_DirHomeIcon)
        self.refresh_button = self._nav_button("Actualizar", QStyle.StandardPixmap.SP_BrowserReload)
        for button in (self.back_button, self.next_button, self.up_button, self.home_button, self.refresh_button):
            nav.addWidget(button)

        self.address = QLineEdit()
        self.address.setPlaceholderText("Ruta de la carpeta (Ctrl+L para editar)")
        self.address.setClearButtonEnabled(True)
        nav.addWidget(self.address, 1)
        self.go_button = QPushButton("Ir")
        nav.addWidget(self.go_button)

        self.search = QLineEdit()
        self.search.setPlaceholderText("Filtrar esta carpeta...")
        self.search.setClearButtonEnabled(True)
        self.search.setMaximumWidth(220)
        nav.addWidget(self.search)
        outer.addLayout(nav)

        self.splitter = QSplitter(Qt.Orientation.Horizontal)
        self.places = QListWidget()
        self.places.setMinimumWidth(160)
        self.places.setMaximumWidth(310)
        self.places.setAlternatingRowColors(False)
        self.splitter.addWidget(self.places)

        self.model = QFileSystemModel(self)
        self.model.setReadOnly(True)
        self.model.setFilter(QDir.Filter.AllEntries | QDir.Filter.NoDotAndDotDot | QDir.Filter.Hidden)
        self.model.setRootPath(QDir.rootPath())
        self.view = QTreeView()
        self.view.setModel(self.model)
        self.view.setRootIsDecorated(False)
        self.view.setItemsExpandable(False)
        self.view.setUniformRowHeights(True)
        self.view.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        self.view.setSelectionMode(QAbstractItemView.SelectionMode.ExtendedSelection)
        self.view.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
        self.view.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.view.setSortingEnabled(True)
        self.view.sortByColumn(0, Qt.SortOrder.AscendingOrder)
        self.view.setColumnWidth(0, 350)
        self.view.setColumnWidth(1, 105)
        self.view.setColumnWidth(2, 130)
        self.view.setColumnWidth(3, 155)
        self.splitter.addWidget(self.view)
        self.splitter.setStretchFactor(0, 0)
        self.splitter.setStretchFactor(1, 1)
        self.splitter.setSizes([210, 810])
        outer.addWidget(self.splitter, 1)

        footer = QHBoxLayout()
        self.status = QLabel("Preparando carpeta...")
        footer.addWidget(self.status, 1)
        self.only_midi = QCheckBox("Mostrar solo MIDI")
        footer.addWidget(self.only_midi)
        outer.addLayout(footer)

        actions = QHBoxLayout()
        actions.addStretch()
        cancel = QPushButton("Cancelar")
        self.accept_button = QPushButton("Importar selección a Añadidos")
        self.accept_button.setDefault(True)
        self.accept_button.setMinimumWidth(215)
        actions.addWidget(cancel)
        actions.addWidget(self.accept_button)
        outer.addLayout(actions)

        self.back_button.clicked.connect(self.go_back)
        self.next_button.clicked.connect(self.go_forward)
        self.up_button.clicked.connect(self.go_up)
        self.home_button.clicked.connect(lambda: self.navigate(os.path.expanduser("~")))
        self.refresh_button.clicked.connect(self.refresh)
        self.go_button.clicked.connect(self.address_navigate)
        self.address.returnPressed.connect(self.address_navigate)
        self.search.textChanged.connect(self.apply_filter)
        self.only_midi.toggled.connect(self.filter_midi)
        self.places.itemClicked.connect(self.open_place)
        self.view.doubleClicked.connect(self.open_item)
        self.view.customContextMenuRequested.connect(self.show_folder_menu)
        self.view.selectionModel().selectionChanged.connect(self.update_status)
        self.model.directoryLoaded.connect(self.directory_loaded)
        self.model.rowsInserted.connect(self.update_status)
        cancel.clicked.connect(self.reject)
        self.accept_button.clicked.connect(self.accept_selection)

        for keys, func in (
            ("Alt+Left", self.go_back), ("Alt+Right", self.go_forward),
            ("Alt+Up", self.go_up),
            ("Ctrl+L", self.focus_address), ("Ctrl+F", self.focus_search),
            ("F5", self.refresh), ("Ctrl+A", self.select_all),
            ("F2", self.rename_selected_folder),
        ):
            shortcut = QShortcut(QKeySequence(keys), self)
            shortcut.activated.connect(func)

        self.rebuild_places()
        saved = str(self.settings.value("last_directory", os.path.expanduser("~")))
        self.navigate(saved if os.path.isdir(saved) else os.path.expanduser("~"))
        QTimer.singleShot(0, self.update_status)

    def _nav_button(self, help_text, symbol):
        button = QPushButton()
        button.setIcon(self.style().standardIcon(symbol))
        button.setToolTip(help_text)
        button.setAccessibleName(help_text)
        button.setFixedWidth(36)
        return button

    def _place(self, label, path, icon):
        path = os.path.normpath(path)
        if not os.path.isdir(path):
            return
        item = QListWidgetItem(self.style().standardIcon(icon), label)
        item.setData(Qt.ItemDataRole.UserRole, path)
        item.setToolTip(path)
        self.places.addItem(item)

    def _heading(self, text):
        item = QListWidgetItem(text)
        font = QFont(item.font())
        font.setBold(True)
        item.setFont(font)
        item.setFlags(Qt.ItemFlag.NoItemFlags)
        self.places.addItem(item)

    def rebuild_places(self):
        self.places.clear()
        self._heading("ACCESO RAPIDO")
        self._place("Carpeta personal", os.path.expanduser("~"), QStyle.StandardPixmap.SP_DirHomeIcon)
        for name, loc in (
            ("Escritorio", QStandardPaths.StandardLocation.DesktopLocation),
            ("Descargas", QStandardPaths.StandardLocation.DownloadLocation),
            ("Documentos", QStandardPaths.StandardLocation.DocumentsLocation),
            ("Musica", QStandardPaths.StandardLocation.MusicLocation),
            ("Imagenes", QStandardPaths.StandardLocation.PicturesLocation),
            ("Videos", QStandardPaths.StandardLocation.MoviesLocation),
        ):
            dirs = QStandardPaths.standardLocations(loc)
            if dirs:
                self._place(name, dirs[0], QStyle.StandardPixmap.SP_DirIcon)

        self._heading("EQUIPO Y UNIDADES")
        self._place("Sistema de archivos (/)", "/", QStyle.StandardPixmap.SP_DriveHDIcon)
        included = {"/"}
        for volume in QStorageInfo.mountedVolumes():
            if not (volume.isValid() and volume.isReady()):
                continue
            path = volume.rootPath()
            if path in included or path.startswith(("/run/", "/sys/", "/proc/", "/dev/", "/boot/")):
                continue
            # Unidades reales y medios montados; no listar tmpfs ni sistemas virtuales.
            dev = bytes(volume.device()).decode("utf-8", "replace")
            if not (path.startswith(("/media/", "/mnt/", "/run/media/")) or dev.startswith("/dev/")):
                continue
            included.add(path)
            name = volume.displayName() or os.path.basename(path)
            self._place(name + " (" + path + ")", path, QStyle.StandardPixmap.SP_DriveHDIcon)
        for root in ("/media", "/run/media", "/mnt"):
            if os.path.isdir(root):
                self._place(root, root, QStyle.StandardPixmap.SP_DirIcon)

    def navigate(self, path, add_history=True):
        path = os.path.abspath(os.path.expandvars(os.path.expanduser(str(path).strip())))
        if os.path.isfile(path):
            file_path = path
            path = os.path.dirname(path)
        else:
            file_path = None
        if not os.path.isdir(path) or not os.access(path, os.R_OK | os.X_OK):
            QMessageBox.warning(self, "Carpeta inaccesible",
                                "No se puede abrir esta carpeta:\n" + path)
            return False
        if add_history:
            if self.history_pos < 0 or self.history[self.history_pos] != path:
                self.history = self.history[:self.history_pos + 1]
                self.history.append(path)
                self.history_pos = len(self.history) - 1
        self.current_dir = path
        self.address.setText(path)
        idx = self.model.index(path)
        self.view.setRootIndex(idx)
        self.view.clearSelection()
        self.search.clear()
        self.back_button.setEnabled(self.history_pos > 0)
        self.next_button.setEnabled(self.history_pos + 1 < len(self.history))
        self.up_button.setEnabled(path != os.path.dirname(path))
        self.settings.setValue("last_directory", path)
        self.update_status()
        if file_path:
            QTimer.singleShot(150, lambda: self.select_path(file_path))
        return True

    def select_path(self, path):
        idx = self.model.index(path)
        if idx.isValid() and os.path.dirname(self.model.filePath(idx)) == self.current_dir:
            self.view.setCurrentIndex(idx)

    def go_back(self):
        if self.history_pos > 0:
            self.history_pos -= 1
            self.navigate(self.history[self.history_pos], add_history=False)

    def go_forward(self):
        if self.history_pos + 1 < len(self.history):
            self.history_pos += 1
            self.navigate(self.history[self.history_pos], add_history=False)

    def go_up(self):
        if self.current_dir:
            self.navigate(os.path.dirname(self.current_dir))

    def address_navigate(self):
        self.navigate(self.address.text())

    def open_place(self, item):
        path = item.data(Qt.ItemDataRole.UserRole)
        if path:
            self.navigate(path)

    def open_item(self, idx):
        path = self.model.filePath(idx)
        if os.path.isdir(path):
            self.navigate(path)
        elif os.path.isfile(path):
            self.accept_selection()

    def show_folder_menu(self, pos):
        idx = self.view.indexAt(pos)
        if not idx.isValid():
            return
        path = self.model.filePath(idx)
        if not os.path.isdir(path) or os.path.islink(path):
            return
        menu = QMenu(self)
        open_action = menu.addAction("Abrir carpeta")
        rename_action = menu.addAction("Renombrar carpeta...")
        menu.addSeparator()
        copy_action = menu.addAction("Copiar ruta")
        chosen = menu.exec(self.view.viewport().mapToGlobal(pos))
        if chosen == open_action:
            self.navigate(path)
        elif chosen == rename_action:
            self.rename_folder(path)
        elif chosen == copy_action:
            QApplication.clipboard().setText(path)

    def rename_selected_folder(self):
        if not self.view.hasFocus():
            return
        selected = self.view.selectionModel().selectedRows(0)
        if len(selected) != 1:
            return
        path = self.model.filePath(selected[0])
        if os.path.isdir(path) and not os.path.islink(path):
            self.rename_folder(path)

    def rename_folder(self, path):
        if not os.path.isdir(path) or os.path.islink(path):
            QMessageBox.warning(self, "Carpeta no válida", "La carpeta ya no existe.")
            return
        path = os.path.normpath(path)
        # Añadidos y sus directorios contenedores no pueden renombrarse:
        # cambiar su ruta rompería la importación protegida de Linthesia.
        data_home = QStandardPaths.writableLocation(
            QStandardPaths.StandardLocation.GenericDataLocation)
        fixed = os.path.realpath(os.path.join(
            data_home, "linthesia", "canciones", "1"))
        actual = os.path.realpath(path)
        if os.path.commonpath((actual, fixed)) == actual:
            QMessageBox.warning(self, "Carpeta protegida",
                                "Añadidos y su ruta de almacenamiento son fijas.")
            return
        parent, old_name = os.path.split(path)
        if not parent or not os.access(parent, os.W_OK | os.X_OK):
            QMessageBox.warning(self, "Solo lectura",
                                "No tienes permisos para renombrar esta carpeta.")
            return
        new_name, accepted = QInputDialog.getText(
            self, "Renombrar carpeta", "Nuevo nombre:", text=old_name)
        if not accepted or new_name == old_name:
            return
        if (not new_name or new_name in (".", "..") or
            new_name != new_name.strip() or
            "/" in new_name or "\\" in new_name or "\x00" in new_name or
            any(ord(char) < 32 or ord(char) == 127 for char in new_name)):
            QMessageBox.warning(self, "Nombre no válido",
                                "Introduce un nombre de carpeta válido.")
            return
        destination = os.path.join(parent, new_name)
        if os.path.lexists(destination):
            QMessageBox.warning(self, "Nombre ya existente",
                                "Ya existe otro elemento con ese nombre.")
            return
        try:
            os.rename(path, destination)
        except OSError as exc:
            QMessageBox.warning(self, "No se pudo renombrar", str(exc))
            return
        QTimer.singleShot(150, lambda: self.select_path(destination))
        self.update_status()

    def refresh(self):
        self.rebuild_places()
        if self.current_dir:
            self.model.setRootPath("")
            self.model.setRootPath(QDir.rootPath())
            self.view.setRootIndex(self.model.index(self.current_dir))
            self.update_status()

    def filter_midi(self, checked):
        self.model.setNameFilters(["*.mid", "*.midi"] if checked else [])
        self.model.setNameFilterDisables(False)
        self.update_status()

    def directory_loaded(self, path):
        if os.path.normpath(path) == self.current_dir:
            self.apply_filter()
        self.update_status()

    def apply_filter(self, *_):
        root = self.view.rootIndex()
        needle = self.search.text().strip().casefold()
        for row in range(self.model.rowCount(root)):
            child = self.model.index(row, 0, root)
            matched = needle in self.model.fileName(child).casefold()
            self.view.setRowHidden(row, root, not matched)
        self.update_status()

    def update_status(self, *_):
        root = self.view.rootIndex()
        count = self.model.rowCount(root) if root.isValid() else 0
        selected = len(self.view.selectionModel().selectedRows(0))
        self.status.setText(f"{count} elementos  |  {selected} seleccionados")

    def focus_address(self):
        self.address.setFocus()
        self.address.selectAll()

    def focus_search(self):
        self.search.setFocus()
        self.search.selectAll()

    def select_all(self):
        if self.address.hasFocus():
            self.address.selectAll()
        elif self.search.hasFocus():
            self.search.selectAll()
        else:
            self.view.setFocus()
            self.view.selectAll()

    def accept_selection(self):
        rows = self.view.selectionModel().selectedRows(0)
        result = []
        invalid = 0
        for idx in rows:
            path = os.path.abspath(self.model.filePath(idx))
            if os.path.isdir(path):
                result.append(path)
            elif os.path.isfile(path) and os.path.splitext(path)[1].lower() in (".mid", ".midi"):
                result.append(path)
            else:
                invalid += 1
        result = list(dict.fromkeys(result))
        if not result:
            text = ("Los archivos seleccionados no son MIDI (.mid o .midi)."
                    if invalid else "Selecciona uno o varios archivos MIDI o carpetas.")
            QMessageBox.information(self, "Sin archivos MIDI", text)
            return
        if invalid:
            answer = QMessageBox.question(
                self, "Archivos no MIDI",
                f"Hay {invalid} archivo(s) que no son MIDI y se ignoraran.\n"
                "¿Deseas importar los archivos MIDI y las carpetas seleccionados?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                QMessageBox.StandardButton.Yes,
            )
            if answer != QMessageBox.StandardButton.Yes:
                return
        self.paths = result
        self.accept()


def main():
    app = QApplication(sys.argv)
    browser = Browser()
    if browser.exec() == QDialog.DialogCode.Accepted:
        for path in browser.paths:
            os.write(1, os.fsencode(path) + b"\0")


if __name__ == "__main__":
    main()
