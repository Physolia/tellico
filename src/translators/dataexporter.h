/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef DATAEXPORTER_H
#define DATAEXPORTER_H

#include "exporter.h"

namespace Bookcase {
  namespace Export {

/**
 * The DataExporter class is meant as an abstract class for any exporter which operates on binary files.
 *
 * @author Robby Stephenson
 * @version $Id: dataexporter.h 817 2004-08-27 07:50:40Z robby $
 */
class DataExporter : public Exporter {
public:
  DataExporter(const Data::Collection* coll) : Exporter(coll) {}

  // not used
  virtual bool exportEntries(bool) const { return false; }
  virtual bool isText() const { return false; }
  /**
   * This should never get called.
   */
  virtual QString text(bool, bool) { return QString::null; }
};

  } // end namespace
} // end namespace
#endif
