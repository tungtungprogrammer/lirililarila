#include "PlaceFile.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace PlaceFile {

static QString vecAttr(const Vec3& v) {
    return QString("%1 %2 %3").arg(static_cast<double>(v.x), 0, 'g', 8).arg(static_cast<double>(v.y), 0, 'g', 8).arg(static_cast<double>(v.z), 0, 'g', 8);
}

static bool parseFloats(const QString& text, float* out, int count) {
    const QStringList p = text.split(' ', Qt::SkipEmptyParts);
    if (p.size() != count) { return false; }
    for (int i = 0; i < count; ++i) { bool ok = false; out[i] = p[i].toFloat(&ok); if (!ok) { return false; } }
    return true;
}

bool save(const Editor& editor, const QString& path, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) { *error = f.errorString(); }
        return false;
    }
    QXmlStreamWriter w(&f);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement("OldBloxPlace");
    w.writeAttribute("version", "1");
    w.writeStartElement("Workspace");
    w.writeAttribute("lighting", QString::number(static_cast<int>(editor.lighting())));
    w.writeAttribute("gravity", QString::number(static_cast<double>(editor.settings().gravity), 'g', 6));
    w.writeAttribute("walkSpeed", QString::number(static_cast<double>(editor.settings().walkSpeed), 'g', 6));
    w.writeAttribute("jumpPower", QString::number(static_cast<double>(editor.settings().jumpPower), 'g', 6));
    w.writeAttribute("maxHealth", QString::number(static_cast<double>(editor.settings().maxHealth), 'g', 6));

    std::vector<Part*> parts;
    editor.workspace()->collectParts(parts);
    for (const Part* p : parts) {
        w.writeStartElement("Part");
        w.writeAttribute("name", QString::fromStdString(p->name));
        w.writeAttribute("brickColor", QString::number(p->brickColor));
        w.writeAttribute("position", vecAttr(p->position));
        QString rot;
        for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) {
            rot += QString::number(static_cast<double>(p->orientation.m[r][c]), 'g', 8) + ((r == 2 && c == 2) ? "" : " ");
        }
        w.writeAttribute("orientation", rot);
        w.writeAttribute("size", vecAttr(p->size));
        w.writeAttribute("anchored", p->anchored ? "true" : "false");
        w.writeAttribute("topSurface", p->topSurface == SurfaceType::Studs ? "Studs" : "Smooth");
        w.writeAttribute("shape", QString::number(static_cast<int>(p->shape)));
        w.writeAttribute("transparency", QString::number(static_cast<double>(p->transparency), 'g', 3));
        w.writeAttribute("locked", p->locked ? "true" : "false");
        w.writeAttribute("canCollide", p->canCollide ? "true" : "false");
        w.writeAttribute("behavior", QString::number(static_cast<int>(p->behavior)));
        w.writeAttribute("behaviorValue", QString::number(static_cast<double>(p->behaviorValue), 'g', 6));
        w.writeEndElement();
    }
    w.writeEndElement();   // Workspace
    w.writeEndElement();   // OldBloxPlace
    w.writeEndDocument();
    if (w.hasError()) { if (error) { *error = "XML write error"; } return false; }
    return true;
}

bool load(Editor& editor, const QString& path, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) { *error = f.errorString(); }
        return false;
    }
    QXmlStreamReader r(&f);
    auto ws = std::make_shared<Workspace>();
    bool sawRoot = false;
    int lighting = 0;
    PlaceSettings settings;
    while (!r.atEnd()) {
        r.readNext();
        if (!r.isStartElement()) { continue; }
        if (r.name() == QLatin1String("OldBloxPlace")) { sawRoot = true; continue; }
        if (r.name() == QLatin1String("Workspace")) {
            lighting = std::max(0, std::min(2, r.attributes().value("lighting").toInt()));
            const QXmlStreamAttributes wa = r.attributes();
            if (wa.hasAttribute("gravity")) settings.gravity = wa.value("gravity").toFloat();
            if (wa.hasAttribute("walkSpeed")) settings.walkSpeed = wa.value("walkSpeed").toFloat();
            if (wa.hasAttribute("jumpPower")) settings.jumpPower = wa.value("jumpPower").toFloat();
            if (wa.hasAttribute("maxHealth")) settings.maxHealth = wa.value("maxHealth").toFloat();
            continue;
        }
        if (r.name() != QLatin1String("Part")) { continue; }

        PartState s;
        const QXmlStreamAttributes a = r.attributes();
        s.name = a.value("name").toString().toStdString();
        if (s.name.empty()) { s.name = "Part"; }
        s.brickColor = a.value("brickColor").toInt();
        float v[9];
        if (parseFloats(a.value("position").toString(), v, 3)) { s.position = Vec3(v[0], v[1], v[2]); }
        if (parseFloats(a.value("size").toString(), v, 3)) { s.size = Vec3(v[0], v[1], v[2]); }
        if (parseFloats(a.value("orientation").toString(), v, 9)) {
            for (int i = 0; i < 9; ++i) { s.orientation.m[i / 3][i % 3] = v[i]; }
        }
        s.anchored = a.value("anchored") != QLatin1String("false");
        s.topSurface = (a.value("topSurface") == QLatin1String("Smooth")) ? SurfaceType::Smooth : SurfaceType::Studs;
        s.shape = static_cast<PartShape>(std::max(0, std::min(3, a.value("shape").toInt())));
        s.transparency = std::max(0.0f, std::min(1.0f, a.value("transparency").toFloat()));
        s.locked = a.value("locked") == QLatin1String("true");
        s.canCollide = a.value("canCollide") != QLatin1String("false");
        s.behavior = static_cast<PartBehavior>(std::max(0, std::min(4, a.value("behavior").toInt())));
        s.behaviorValue = a.value("behaviorValue").toFloat();
        ws->createPart(s);
    }
    if (r.hasError() || !sawRoot) {
        if (error) { *error = r.hasError() ? r.errorString() : QString("Not an OldBlox place file"); }
        return false;
    }
    editor.resetPlace(ws, path.toStdString());
    editor.setLighting(static_cast<Lighting>(lighting));
    editor.loadSettings(settings);
    editor.setModified(false);
    return true;
}

}  // namespace PlaceFile
