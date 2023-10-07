/*************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#include <config.h>

#include "bibtexmlexporter.h"
#include "../utils/bibtexhandler.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "tellico_xml.h"
#include "../utils/stringset.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QDomDocument>
#include <QTextCodec>

using namespace Tellico;
using Tellico::Export::BibtexmlExporter;

BibtexmlExporter::BibtexmlExporter(Data::CollPtr coll_) : Exporter(coll_) {
}

QString BibtexmlExporter::formatString() const {
  return QStringLiteral("Bibtexml");
}

QString BibtexmlExporter::fileFilter() const {
  return i18n("Bibtexml Files") + QLatin1String(" (*.xml)") + QLatin1String(";;") + i18n("All Files") + QLatin1String(" (*)");
}

bool BibtexmlExporter::exec() {
  const QString text = this->text();
  return text.isEmpty() ? false : FileHandler::writeTextURL(url(), text, options() & ExportUTF8, options() & Export::ExportForce);
}

QString BibtexmlExporter::text() {
  Data::CollPtr c = collection();
  if(!c || c->type() != Data::Collection::Bibtex) {
    return QString();
  }

// there are some special fields
// the entry-type specifies the entry type - book, inproceedings, whatever
  QString typeField;
// the key specifies the cite-key
  QString keyField;

  const QString bibtex = QStringLiteral("bibtex");
// keep a list of all the 'ordinary' fields to iterate through later
  Data::FieldList fields;
  foreach(Data::FieldPtr it, this->fields()) {
    QString bibtexField = it->property(bibtex);
    if(bibtexField == QLatin1String("entry-type")) {
      typeField = it->name();
    } else if(bibtexField == QLatin1String("key")) {
      keyField = it->name();
    } else if(!bibtexField.isEmpty()) {
      fields.append(it);
    }
  }

  QDomImplementation impl;
  QDomDocumentType doctype = impl.createDocumentType(QStringLiteral("file"),
                                                     QString(),
                                                     XML::dtdBibtexml);
  //default namespace
  const QString& ns = XML::nsBibtexml;

  QDomDocument dom = impl.createDocument(ns, QStringLiteral("file"), doctype);

  // root element
  QDomElement root = dom.documentElement();

  QString encodeStr = QStringLiteral("version=\"1.0\" encoding=\"");
  if(options() & Export::ExportUTF8) {
    encodeStr += QLatin1String("UTF-8");
  } else {
    encodeStr += QLatin1String(QTextCodec::codecForLocale()->name());
  }
  encodeStr += QLatin1Char('"');

  // createDocument creates a root node, insert the processing instruction before it
  dom.insertBefore(dom.createProcessingInstruction(QStringLiteral("xml"), encodeStr), root);
  QString comment = QLatin1String("Generated by Tellico ") + QLatin1String(TELLICO_VERSION);
  dom.insertBefore(dom.createComment(comment), root);

  FieldFormat::Request format = (options() & Export::ExportFormatted ?
                                                FieldFormat::ForceFormat :
                                                FieldFormat::AsIsFormat);

  StringSet usedKeys;
  QString type, key, newKey, value, elemName, parElemName;
  QDomElement btElem, entryElem, parentElem, fieldElem;
  foreach(Data::EntryPtr entryIt, entries()) {
    key = entryIt->field(keyField);
    if(key.isEmpty()) {
      key = BibtexHandler::bibtexKey(entryIt);
    }
    newKey = key;
    char c = 'a';
    while(usedKeys.has(newKey)) {
      // duplicate found!
      newKey = key + QLatin1Char(c);
      ++c;
    }
    key = newKey;
    usedKeys.add(key);

    btElem = dom.createElement(QStringLiteral("entry"));
    btElem.setAttribute(QStringLiteral("id"), key);
    root.appendChild(btElem);

    type = entryIt->field(typeField);
    if(type.isEmpty()) {
      myWarning() << "the entry for '" << entryIt->title()
                 << "' has no entry-type, skipping it!";
      continue;
    }

    entryElem = dom.createElement(type);
    btElem.appendChild(entryElem);

    // now iterate over attributes
    foreach(Data::FieldPtr field, fields) {
      value = entryIt->formattedField(field, format);
      if(value.isEmpty()) {
        continue;
      }

/* Bibtexml has special container elements for titles, authors, editors, and keywords
   I'm going to ignore the titlelist element for right now. All authors are contained in
   an authorlist element, editors in an editorlist element, and keywords are in a
   keywords element, and themselves as a keyword. Also, Bibtexml can format names
   similar to docbook, with first, middle, last, etc elements. I'm going to ignore that
   for now, too.*/
      elemName = field->property(bibtex);
      // split text for author, editor, and keywords
      if(elemName == QLatin1String("author") ||
         elemName == QLatin1String("editor") ||
         elemName == QLatin1String("keywords")) {
        if(elemName == QLatin1String("author")) {
          parElemName = QStringLiteral("authorlist");
        } else if(elemName == QLatin1String("editor")) {
          parElemName = QStringLiteral("editorlist");
        } else { // keywords
          parElemName = QStringLiteral("keywords");
          elemName = QStringLiteral("keyword");
        }

        parentElem = dom.createElement(parElemName);
        const QStringList values = FieldFormat::splitValue(entryIt->formattedField(field, format));
        foreach(const QString& value, values) {
          fieldElem = dom.createElement(elemName);
          fieldElem.appendChild(dom.createTextNode(value));
          parentElem.appendChild(fieldElem);
        }
        if(parentElem.hasChildNodes()) {
          entryElem.appendChild(parentElem);
        }
      } else {
        fieldElem = dom.createElement(elemName);
        fieldElem.appendChild(dom.createTextNode(value));
        entryElem.appendChild(fieldElem);
      }
    }
  }

  return dom.toString();
}
