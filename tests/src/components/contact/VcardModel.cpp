#include <belcard/belcard.hpp>
#include <QFileInfo>
#include <QImageReader>
#include <QtDebug>
#include <QUuid>

#include "../../app/AvatarProvider.hpp"
#include "../../app/Database.hpp"
#include "../../utils.hpp"
#include "../core/CoreManager.hpp"

#include "VcardModel.hpp"

#define VCARD_SCHEME "linphone-desktop:/"

using namespace std;

// =============================================================================

template<class T>
inline shared_ptr<T> findBelCardValue (const list<shared_ptr<T> > &list, const QString &value) {
  string match = ::Utils::qStringToLinphoneString(value);

  auto it = find_if(
      list.cbegin(), list.cend(), [&match](const shared_ptr<T> &entry) {
        return match == entry->getValue();
      }
    );

  return *it;
}

// -----------------------------------------------------------------------------

QString VcardModel::getUsername () const {
  return ::Utils::linphoneStringToQString(m_vcard->getFullName());
}

void VcardModel::setUsername (const QString &username) {
  if (username.length() == 0 || username == getUsername())
    return;

  m_vcard->setFullName(::Utils::qStringToLinphoneString(username));
  emit vcardUpdated();
}

// -----------------------------------------------------------------------------

QString VcardModel::getAvatar () const {
  // Find desktop avatar.
  list<shared_ptr<belcard::BelCardPhoto> > photos = m_vcard->getBelcard()->getPhotos();
  auto it = find_if(
      photos.cbegin(), photos.cend(), [](const shared_ptr<belcard::BelCardPhoto> &photo) {
        return !photo->getValue().compare(0, sizeof(VCARD_SCHEME) - 1, VCARD_SCHEME);
      }
    );

  // No path found.
  if (it == photos.cend())
    return "";

  // Returns right path.
  return QStringLiteral("image://%1/%2").arg(AvatarProvider::PROVIDER_ID).arg(
    ::Utils::linphoneStringToQString((*it)->getValue().substr(sizeof(VCARD_SCHEME) - 1))
  );
}

void VcardModel::setAvatar (const QString &path) {
  // 1. Try to copy photo in avatars folder.
  QFile file(path);

  if (!file.exists() || QImageReader::imageFormat(path).size() == 0)
    return;

  QFileInfo info(file);
  QString uuid = QUuid::createUuid().toString();
  QString file_id = QStringLiteral("%1.%2")
    .arg(uuid.mid(1, uuid.length() - 2)) // Remove `{}`.
    .arg(info.suffix());

  QString dest = ::Utils::linphoneStringToQString(Database::getAvatarsPath()) + file_id;

  if (!file.copy(dest))
    return;

  qInfo() << QStringLiteral("Update avatar of `%1`. (path=%2)").arg(getUsername()).arg(dest);

  // 2. Edit vcard.
  shared_ptr<belcard::BelCard> belCard = m_vcard->getBelcard();
  list<shared_ptr<belcard::BelCardPhoto> > photos = belCard->getPhotos();

  // 3. Remove oldest photo.
  auto it = find_if(
      photos.begin(), photos.end(), [](const shared_ptr<belcard::BelCardPhoto> &photo) {
        return !photo->getValue().compare(0, sizeof(VCARD_SCHEME) - 1, VCARD_SCHEME);
      }
    );

  if (it != photos.end()) {
    QString image_path(
      ::Utils::linphoneStringToQString(
        Database::getAvatarsPath() + (*it)->getValue().substr(sizeof(VCARD_SCHEME) - 1)
      )
    );

    if (!QFile::remove(image_path))
      qWarning() << QStringLiteral("Unable to remove `%1`.").arg(image_path);
    belCard->removePhoto(*it);
  }

  // 4. Update.
  shared_ptr<belcard::BelCardPhoto> photo =
    belcard::BelCardGeneric::create<belcard::BelCardPhoto>();
  photo->setValue(VCARD_SCHEME + ::Utils::qStringToLinphoneString(file_id));
  belCard->addPhoto(photo);

  emit vcardUpdated();
}

// -----------------------------------------------------------------------------

QVariantMap VcardModel::getAddress () const {
  // TODO
}

void VcardModel::setAddress (const QVariantMap &address) {
  // TODO
}

QVariantList VcardModel::getSipAddresses () const {
  QVariantList list;

  for (const auto &address : m_vcard->getSipAddresses())
    list.append(::Utils::linphoneStringToQString(address->asString()));

  return list;
}

bool VcardModel::addSipAddress (const QString &sip_address) {
  qInfo() << QStringLiteral("Add new sip address: `%1`.").arg(sip_address);
  m_vcard->addSipAddress(::Utils::qStringToLinphoneString(sip_address));

  emit vcardUpdated();

  return true;
}

void VcardModel::removeSipAddress (const QString &sip_address) {
  list<shared_ptr<linphone::Address> > addresses = m_vcard->getSipAddresses();
  string match = ::Utils::qStringToLinphoneString(sip_address);

  auto it = find_if(
      addresses.cbegin(), addresses.cend(), [&match](const shared_ptr<linphone::Address> &address) {
        return match == address->asString();
      }
    );

  if (it == addresses.cend()) {
    qWarning() << QStringLiteral("Unable to found sip address: `%1`.")
      .arg(sip_address);
    return;
  }

  if (addresses.size() == 1) {
    qWarning() << QStringLiteral("Unable to remove the only existing sip address: `%1`.")
      .arg(sip_address);
    return;
  }

  qInfo() << QStringLiteral("Remove sip address: `%1`.").arg(sip_address);
  m_vcard->removeSipAddress((*it)->asStringUriOnly());

  emit vcardUpdated();
}

bool VcardModel::updateSipAddress (const QString &old_sip_address, const QString &sip_address) {
  if (old_sip_address == sip_address || !addSipAddress(sip_address))
    return false;

  removeSipAddress(old_sip_address);

  return true;
}

// -----------------------------------------------------------------------------

QVariantList VcardModel::getCompanies () const {
  QVariantList list;

  for (const auto &company : m_vcard->getBelcard()->getRoles())
    list.append(::Utils::linphoneStringToQString(company->getValue()));

  return list;
}

bool VcardModel::addCompany (const QString &company) {
  shared_ptr<belcard::BelCard> belCard = m_vcard->getBelcard();
  shared_ptr<belcard::BelCardRole> value = belcard::BelCardGeneric::create<belcard::BelCardRole>();
  value->setValue(::Utils::qStringToLinphoneString(company));

  qInfo() << QStringLiteral("Add new company: `%1`.").arg(company);
  belCard->addRole(value);

  emit vcardUpdated();

  return true;
}

void VcardModel::removeCompany (const QString &company) {
  shared_ptr<belcard::BelCard> belCard = m_vcard->getBelcard();
  shared_ptr<belcard::BelCardRole> value = findBelCardValue(belCard->getRoles(), company);

  if (!value) {
    qWarning() << QStringLiteral("Unable to remove company: `%1`.").arg(company);
    return;
  }

  qInfo() << QStringLiteral("Remove company: `%1`.").arg(company);
  belCard->removeRole(value);

  emit vcardUpdated();
}

bool VcardModel::updateCompany (const QString &old_company, const QString &company) {
  if (old_company == company || !addCompany(company))
    return false;

  removeCompany(old_company);

  return true;
}

// -----------------------------------------------------------------------------

QVariantList VcardModel::getEmails () const {
  QVariantList list;

  for (const auto &email : m_vcard->getBelcard()->getEmails())
    list.append(::Utils::linphoneStringToQString(email->getValue()));

  return list;
}

bool VcardModel::addEmail (const QString &email) {
  shared_ptr<belcard::BelCard> belCard = m_vcard->getBelcard();
  shared_ptr<belcard::BelCardEmail> value =
    belcard::BelCardGeneric::create<belcard::BelCardEmail>();
  value->setValue(::Utils::qStringToLinphoneString(email));

  qInfo() << QStringLiteral("Add new email: `%1`.").arg(email);
  belCard->addEmail(value);

  emit vcardUpdated();

  return true;
}

void VcardModel::removeEmail (const QString &email) {
  shared_ptr<belcard::BelCard> belCard = m_vcard->getBelcard();
  shared_ptr<belcard::BelCardEmail> value = findBelCardValue(belCard->getEmails(), email);

  if (!value) {
    qWarning() << QStringLiteral("Unable to remove email: `%1`.").arg(email);
    return;
  }

  qInfo() << QStringLiteral("Remove email: `%1`.").arg(email);
  belCard->removeEmail(value);

  emit vcardUpdated();
}

bool VcardModel::updateEmail (const QString &old_email, const QString &email) {
  if (old_email == email || !addEmail(email))
    return false;

  removeEmail(old_email);

  return true;
}

// -----------------------------------------------------------------------------

QVariantList VcardModel::getUrls () const {
  QVariantList list;

  for (const auto &url : m_vcard->getBelcard()->getURLs())
    list.append(::Utils::linphoneStringToQString(url->getValue()));

  return list;
}

bool VcardModel::addUrl (const QString &url) {
  shared_ptr<belcard::BelCard> belCard = m_vcard->getBelcard();
  shared_ptr<belcard::BelCardURL> value = belcard::BelCardGeneric::create<belcard::BelCardURL>();
  value->setValue(::Utils::qStringToLinphoneString(url));

  qInfo() << QStringLiteral("Add new url: `%1`.").arg(url);
  belCard->addURL(value);

  emit vcardUpdated();

  return true;
}

void VcardModel::removeUrl (const QString &url) {
  shared_ptr<belcard::BelCard> belCard = m_vcard->getBelcard();
  shared_ptr<belcard::BelCardURL> value = findBelCardValue(belCard->getURLs(), url);

  if (!value) {
    qWarning() << QStringLiteral("Unable to remove url: `%1`.").arg(url);
    return;
  }

  qInfo() << QStringLiteral("Remove url: `%1`.").arg(url);
  belCard->removeURL(value);

  emit vcardUpdated();
}

bool VcardModel::updateUrl (const QString &old_url, const QString &url) {
  if (old_url == url || !addUrl(url))
    return false;

  removeUrl(old_url);

  return true;
}