import sys
from PyQt6.QtCore import QPluginLoader, QCoreApplication, QDir

app = QCoreApplication(sys.argv)
paths = QCoreApplication.libraryPaths()
print("Library paths:", paths)
