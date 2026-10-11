#pragma once
#include <QIcon>
#include <QPixmap>

// All icons are drawn in code (16x16 logical pixels) so the project needs no image files.
namespace Icons {

enum Id { New, Open, Save, Cut, Copy, Paste, Undo, Redo, Select, Move, Resize, Delete, Workspace, Brick, About };

QIcon icon(Id id);
QIcon partIcon(int brickColorId);                 // little cube tinted with the BrickColor
QPixmap swatch(int brickColorId, int size = 14);  // flat colour chip for combo boxes

}  // namespace Icons
