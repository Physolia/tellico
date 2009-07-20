/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#undef QT_NO_CAST_FROM_ASCII

#include "qtest_kde.h"
#include "citebasefetchertest.h"
#include "citebasefetchertest.moc"

#include "../fetch/fetcherjob.h"
#include "../fetch/citebasefetcher.h"
#include "../entry.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"

#include <kstandarddirs.h>

QTEST_KDEMAIN_CORE( CitebaseFetcherTest )

namespace {
  // 5 is BibtexCollection
  static const int COLLECTION_TYPE = 5;
}

CitebaseFetcherTest::CitebaseFetcherTest() : m_loop(this) {
}

void CitebaseFetcherTest::initTestCase() {
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");

  m_fieldValues.insert(QLatin1String("arxiv"), QLatin1String("hep-lat/0110180"));
  m_fieldValues.insert(QLatin1String("entry-type"), QLatin1String("article"));
  m_fieldValues.insert(QLatin1String("title"), QLatin1String("Speeding up the Hybrid-Monte-Carlo algorithm for dynamical fermions"));
  m_fieldValues.insert(QLatin1String("author"), QLatin1String("M. Hasenbusch; K. Jansen"));
  m_fieldValues.insert(QLatin1String("keyword"), QLatin1String("High Energy Physics - Lattice"));
  m_fieldValues.insert(QLatin1String("journal"), QLatin1String("Nucl.Phys.Proc.Suppl. 106"));
  m_fieldValues.insert(QLatin1String("year"), QLatin1String("2002"));
  m_fieldValues.insert(QLatin1String("pages"), QLatin1String("1076-1078"));
}

void CitebaseFetcherTest::testArxivID() {
  Tellico::Fetch::FetchRequest request(COLLECTION_TYPE, Tellico::Fetch::ArxivID, "arxiv:" + m_fieldValues.value("arxiv"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::CitebaseFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  Tellico::Data::EntryList results = job->entries();
  QEXPECT_FAIL("", "citebase.org is currently down", Continue);
  QCOMPARE(results.size(), 1);

  if(!results.isEmpty()) {
    Tellico::Data::EntryPtr entry = results.at(0);
    QHashIterator<QString, QString> i(m_fieldValues);
    while(i.hasNext()) {
      i.next();
      QCOMPARE(entry->field(i.key()), i.value());
    }
  }
}

void CitebaseFetcherTest::testArxivIDVersioned() {
  QString arxivVersioned = m_fieldValues.value("arxiv") + "v1";
  Tellico::Fetch::FetchRequest request(COLLECTION_TYPE, Tellico::Fetch::ArxivID, arxivVersioned);
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::CitebaseFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  Tellico::Data::EntryList results = job->entries();
  QEXPECT_FAIL("", "citebase.org is currently down", Continue);
  QCOMPARE(results.size(), 1);

  if(!results.isEmpty()) {
    Tellico::Data::EntryPtr entry = results.at(0);
    // id has version since original search included it
    QCOMPARE(entry->field("arxiv"), arxivVersioned);
    QHashIterator<QString, QString> i(m_fieldValues);
    while(i.hasNext()) {
      i.next();
      if(i.key() == "arxiv") continue;
      QCOMPARE(entry->field(i.key()), i.value());
    }
  }
}

void CitebaseFetcherTest::slotResult(KJob*) {
  m_loop.quit();
}
