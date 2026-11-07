#include "DictionariesActivity.h"

#include <HalPowerManager.h>
#include <I18n.h>

#include <cstdio>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "activities/util/ConfirmationActivity.h"
#include "fontIds.h"
#include "network/OnlineDictionary.h"
#include "util/Dictionary.h"
#include "util/DictionaryRegistry.h"

namespace {

// Keep Wi-Fi on next to a book only where the memory allows it (as before).
constexpr bool kWifiRowShown = HalPowerManager::LOW_POWER_FREQ >= 80;

}  // namespace

DictionariesActivity::DictionariesActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : RowListActivity(renderer, mappedInput, "Dictionaries", tr(STR_DICTIONARIES)) {}

void DictionariesActivity::onEnter() {
  Activity::onEnter();
  // The registry is filled on demand: scan the card now (as the dictionary
  // picker does) and free it again in onExit().
  dictionaryRegistry.discover();
  dictionaries.clear();
  // The dictionaries on the card and the language of each (its .ifo lang=).
  for (const auto& entry : dictionaryRegistry.getEntries()) {
    CardDictionary card;
    card.name = entry.name;
    card.path = entry.basePath;
    const DictInfo info = Dictionary::readInfo(entry.basePath.c_str());
    card.langKnown = info.valid && onlinedict::langFromCode(info.lang, card.lang);
    card.sameLanguage = card.langKnown && info.lang[2] == '-' && info.lang[3] == info.lang[0] &&
                        info.lang[4] == info.lang[1];
    dictionaries.push_back(std::move(card));
  }
  for (uint8_t i = 0; i < onlinedict::kLangCount; ++i) {
    const auto lang = static_cast<onlinedict::Lang>(i);
    offlinePaths[i] = Dictionary::readLanguageDictPath(onlinedict::code(lang));
    if (!offlinePaths[i].empty()) continue;
    // Nothing chosen yet: take this language's dictionary from the card,
    // one in the same language ("de-de") before a translating one ("de-en").
    const CardDictionary* best = nullptr;
    for (const auto& card : dictionaries) {
      if (!card.langKnown || card.lang != lang) continue;
      if (!best || (card.sameLanguage && !best->sameLanguage)) best = &card;
    }
    if (best) chooseOffline(lang, best->path);
  }
  if (chosenLanguages().empty()) SETTINGS.readingLanguages = 1;  // English
  selected = 0;
  RowListActivity::onEnter();
}

void DictionariesActivity::onExit() {
  dictionaryRegistry.clear();
  RowListActivity::onExit();
}

void DictionariesActivity::chooseOffline(const onlinedict::Lang lang, const std::string& path) {
  offlinePaths[static_cast<uint8_t>(lang)] = path;
  Dictionary::saveLanguageDictPath(onlinedict::code(lang), path.c_str());
  // Books in a language without its own choice use the general dictionary.
  if (!path.empty() && Dictionary::readConfiguredDictPath().empty()) Dictionary::saveGlobalDictPath(path.c_str());
}

std::vector<onlinedict::Lang> DictionariesActivity::chosenLanguages() const {
  std::vector<onlinedict::Lang> out;
  for (uint8_t i = 0; i < onlinedict::kLangCount; ++i) {
    if (SETTINGS.readingLanguages & (1u << i)) out.push_back(static_cast<onlinedict::Lang>(i));
  }
  return out;
}

onlinedict::Lang DictionariesActivity::current() const {
  const auto chosen = chosenLanguages();
  if (chosen.empty()) return onlinedict::Lang::English;
  return chosen[selected < static_cast<int>(chosen.size()) ? selected : 0];
}

bool DictionariesActivity::hasCardDictionary(const onlinedict::Lang lang) const {
  for (const auto& card : dictionaries) {
    if (card.langKnown && card.lang == lang) return true;
  }
  return false;
}

std::string DictionariesActivity::offlineName(const onlinedict::Lang lang) const {
  const std::string& path = offlinePaths[static_cast<uint8_t>(lang)];
  for (const auto& card : dictionaries) {
    if (card.path == path) return card.name;
  }
  return tr(STR_DICT_NONE);
}

void DictionariesActivity::openPicker(const int row) {
  const onlinedict::Lang lang = current();
  std::vector<std::string> options;
  pickerValues.clear();
  int currentIndex = 0;
  if (row == Offline) {
    const std::string& chosenPath = offlinePaths[static_cast<uint8_t>(lang)];
    options.push_back(tr(STR_DICT_NONE));
    pickerValues.push_back(-1);
    // This language's dictionaries first, then the rest.
    for (int pass = 0; pass < 2; ++pass) {
      for (size_t i = 0; i < dictionaries.size(); ++i) {
        const bool matches = dictionaries[i].langKnown && dictionaries[i].lang == lang;
        if (matches != (pass == 0)) continue;
        if (dictionaries[i].path == chosenPath) currentIndex = static_cast<int>(options.size());
        options.push_back(dictionaries[i].name);
        pickerValues.push_back(static_cast<int>(i));
      }
    }
    popup.show(StrId::STR_DICT_OFFLINE, options, currentIndex, [this, lang](const int index) {
      const int card = pickerValues[index];
      chooseOffline(lang, card < 0 ? std::string() : dictionaries[card].path);
      refresh();
    });
    requestUpdate();  // without it the popup is open but never drawn, and eats every tap
    return;
  }
  const onlinedict::Provider first = OnlineDictionary::preferredProvider(lang);
  const onlinedict::Provider second = OnlineDictionary::secondProvider(lang);
  const onlinedict::Provider now = row == OnlineFirst ? first : second;
  for (uint8_t p = 0; p < onlinedict::kProviderCount; ++p) {
    const auto provider = static_cast<onlinedict::Provider>(p);
    if (!onlinedict::providerServes(provider, lang)) continue;
    if (provider == now) currentIndex = static_cast<int>(options.size());
    options.push_back(onlinedict::providerName(provider));
    pickerValues.push_back(p);
  }
  if (row == OnlineSecond) {
    if (now == onlinedict::Provider::None) currentIndex = static_cast<int>(options.size());
    options.push_back(tr(STR_SRC_NONE));
    pickerValues.push_back(static_cast<int>(onlinedict::Provider::None));
  }
  popup.show(row == OnlineFirst ? StrId::STR_DICT_ONLINE_FIRST : StrId::STR_DICT_ONLINE_SECOND, options,
             currentIndex, [this, lang, row, first, second](const int index) {
               const auto picked = static_cast<onlinedict::Provider>(pickerValues[index]);
               if (row == OnlineFirst) {
                 // Picking the second source as first swaps them.
                 OnlineDictionary::setSources(lang, picked, picked == second ? first : second);
               } else {
                 OnlineDictionary::setSources(lang, first, picked);
               }
               refresh();
             });
  requestUpdate();
}

void DictionariesActivity::openLanguagePicker() {
  const auto chosen = chosenLanguages();
  std::vector<std::string> options;
  for (const auto lang : chosen) options.push_back(onlinedict::nativeName(lang));
  popup.show(StrId::STR_LANGUAGE, options, selected, [this](const int index) {
    selected = index;
    refresh();
  });
  requestUpdate();
}

void DictionariesActivity::openAddLanguage() {
  std::vector<std::string> options;
  pickerValues.clear();
  for (uint8_t i = 0; i < onlinedict::kLangCount; ++i) {
    if (SETTINGS.readingLanguages & (1u << i)) continue;
    options.push_back(onlinedict::nativeName(static_cast<onlinedict::Lang>(i)));
    pickerValues.push_back(i);
  }
  if (options.empty()) return;
  popup.show(StrId::STR_DICT_ADD_LANGUAGE, options, 0, [this](const int index) {
    const int lang = pickerValues[index];
    SETTINGS.readingLanguages = static_cast<uint16_t>(SETTINGS.readingLanguages | (1u << lang));
    SETTINGS.saveToFile();
    const auto chosen = chosenLanguages();
    for (size_t i = 0; i < chosen.size(); ++i) {
      if (static_cast<int>(chosen[i]) == lang) selected = static_cast<int>(i);
    }
    refresh();
  });
  requestUpdate();
}

void DictionariesActivity::removeLanguage(const int index) {
  const auto chosen = chosenLanguages();
  if (chosen.size() <= 1 || index >= static_cast<int>(chosen.size())) return;  // one language always stays
  SETTINGS.readingLanguages =
      static_cast<uint16_t>(SETTINGS.readingLanguages & ~(1u << static_cast<uint8_t>(chosen[index])));
  SETTINGS.saveToFile();
  selected = 0;
}

void DictionariesActivity::toggleWifi() {
  if (SETTINGS.onlineDictWifiAlwaysOn != 0) {
    SETTINGS.onlineDictWifiAlwaysOn = 0;
    SETTINGS.saveToFile();
    return;
  }
  // Same warning as before: Wi-Fi next to a book costs battery.
  startActivityForResult(std::make_unique<ConfirmationActivity>(renderer, mappedInput, tr(STR_KEEP_WIFI_IN_BOOKS),
                                                                tr(STR_ONLINE_WIFI_WARNING)),
                         [this](const ActivityResult& result) {
                           if (!result.isCancelled) {
                             SETTINGS.onlineDictWifiAlwaysOn = 1;
                             SETTINGS.saveToFile();
                           }
                           refresh();
                         });
}


void DictionariesActivity::buildRows(std::vector<Row>& rows) {
  const onlinedict::Lang lang = current();
  const auto chosen = chosenLanguages();
  const auto add = [&rows](Row::Kind kind, std::string label, std::string value, std::function<void()> onTap,
                           std::string subtitle = {}) {
    Row row;
    row.kind = kind;
    row.label = std::move(label);
    row.value = std::move(value);
    row.subtitle = std::move(subtitle);
    row.onTap = std::move(onTap);
    rows.push_back(std::move(row));
  };
  rows.clear();

  add(Row::Kind::Header, tr(STR_LANGUAGE), "", nullptr);
  add(Row::Kind::Value, onlinedict::nativeName(lang), "", chosen.size() > 1 ? std::function<void()>([this] { openLanguagePicker(); }) : nullptr);
  add(Row::Kind::Chevron, tr(STR_DICT_ADD_LANGUAGE), "", [this] { openAddLanguage(); });
  if (chosen.size() > 1) add(Row::Kind::Chevron, tr(STR_DICT_REMOVE_LANGUAGE), "", [this] { removeLanguage(selected); });

  add(Row::Kind::Header, onlinedict::nativeName(lang), "", nullptr);
  std::string note;
  if (!hasCardDictionary(lang)) {
    // No dictionary folder on the card yet: name the one the scan looks for.
    const std::string& root = dictionaryRegistry.root();
    char text[160];
    snprintf(text, sizeof(text), tr(STR_DICT_NO_CARD_DICT), onlinedict::nativeName(lang),
             root.empty() ? "/dictionaries" : root.c_str());
    note = text;
  }
  add(Row::Kind::Value, tr(STR_DICT_OFFLINE), offlineName(lang), [this] { openPicker(Offline); }, note);
  add(Row::Kind::Value, tr(STR_DICT_ONLINE_FIRST), onlinedict::providerName(OnlineDictionary::preferredProvider(lang)),
      [this] { openPicker(OnlineFirst); });
  const onlinedict::Provider second = OnlineDictionary::secondProvider(lang);
  add(Row::Kind::Value, tr(STR_DICT_ONLINE_SECOND),
      second == onlinedict::Provider::None ? tr(STR_SRC_NONE) : onlinedict::providerName(second),
      [this] { openPicker(OnlineSecond); });

  if (kWifiRowShown) {
    add(Row::Kind::Header, tr(STR_DICT_SECTION_ONLINE), "", nullptr);
    add(Row::Kind::Toggle, tr(STR_KEEP_WIFI_IN_BOOKS), "", [this] { toggleWifi(); });
    rows.back().checked = SETTINGS.onlineDictWifiAlwaysOn != 0;
  }
}
