#pragma once
#include <QString>
#include "DataModel/Editor.h"

// Saves/loads a place as XML (".obxl"), in the spirit of the original .rbxl files.
namespace PlaceFile {
bool save(const Editor& editor, const QString& path, QString* error);
bool load(Editor& editor, const QString& path, QString* error);
}
