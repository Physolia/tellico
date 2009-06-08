/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#include "bibsonomyfetcher.h"
#include "messagehandler.h"
#include "searchresult.h"
#include "../translators/bibteximporter.h"
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../collection.h"
#include "../entry.h"
#include "../core/netaccess.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>

#include <QLabel>
#include <QVBoxLayout>

namespace {
  // always bibtex
  static const char* BIBSONOMY_BASE_URL = "http://bibsonomy.org";
  static const int BIBSONOMY_MAX_RESULTS = 20;
}

using Tellico::Fetch::BibsonomyFetcher;

BibsonomyFetcher::BibsonomyFetcher(QObject* parent_)
    : Fetcher(parent_), m_job(0), m_started(false) {
}

BibsonomyFetcher::~BibsonomyFetcher() {
}

QString BibsonomyFetcher::defaultName() {
  return QLatin1String("Bibsonomy");
}

QString BibsonomyFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool BibsonomyFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void BibsonomyFetcher::readConfigHook(const KConfigGroup&) {
}

void BibsonomyFetcher::search(Tellico::Fetch::FetchKey key_, const QString& value_) {
  m_key = key_;
  m_value = value_.trimmed();
  m_started = true;

  if(!canFetch(collectionType())) {
    message(i18n("%1 does not allow searching for this collection type.", source()), MessageHandler::Warning);
    stop();
    return;
  }

//  myDebug() << "value = " << value_;

  KUrl u = searchURL(m_key, m_value);
  if(u.isEmpty()) {
    stop();
    return;
  }

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
}

void BibsonomyFetcher::stop() {
  if(!m_started) {
    return;
  }
//  myDebug();
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_started = false;
  emit signalDone(this);
}

void BibsonomyFetcher::slotComplete(KJob*) {
//  myDebug();

  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  Import::BibtexImporter imp(QString::fromUtf8(data, data.size()));
  Data::CollPtr coll = imp.collection();

  if(!coll) {
    myDebug() << "no valid result";
    stop();
    return;
  }

  Data::EntryList entries = coll->entries();
  foreach(Data::EntryPtr entry, entries) {
    if(!m_started) {
      // might get aborted
      break;
    }

    SearchResult* r = new SearchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
  }

  stop(); // required
}

Tellico::Data::EntryPtr BibsonomyFetcher::fetchEntry(uint uid_) {
  return m_entries[uid_];
}

KUrl BibsonomyFetcher::searchURL(Tellico::Fetch::FetchKey key_, const QString& value_) const {
  KUrl u(BIBSONOMY_BASE_URL);
  u.setPath(QLatin1String("/bib/"));

  switch(key_) {
    case Person:
      u.addPath(QString::fromLatin1("author/%1").arg(value_));
      break;

    case Keyword:
      u.addPath(QString::fromLatin1("search/%1").arg(value_));
      break;

    default:
      myWarning() << "key not recognized: " << m_key;
      return KUrl();
  }

  u.addQueryItem(QLatin1String("items"), QString::number(BIBSONOMY_MAX_RESULTS));
  myDebug() << "url: " << u.url();
  return u;
}

void BibsonomyFetcher::updateEntry(Tellico::Data::EntryPtr entry_) {
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    search(Fetch::Keyword, title);
    return;
  }

  myDebug() << "insufficient info to search";
  emit signalDone(this); // always need to emit this if not continuing with the search
}

Tellico::Fetch::ConfigWidget* BibsonomyFetcher::configWidget(QWidget* parent_) const {
  return new BibsonomyFetcher::ConfigWidget(parent_, this);
}

BibsonomyFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const BibsonomyFetcher*)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

void BibsonomyFetcher::ConfigWidget::saveConfig(KConfigGroup&) {
}

QString BibsonomyFetcher::ConfigWidget::preferredName() const {
  return BibsonomyFetcher::defaultName();
}

#include "bibsonomyfetcher.moc"
