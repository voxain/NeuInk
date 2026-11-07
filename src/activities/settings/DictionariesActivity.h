#pragma once

#include <string>
#include <vector>

#include "RowListActivity.h"
#include "util/OnlineDictionaryText.h"

// VARAGH 1.3.0: Settings > Dictionaries. One page for everything a lookup uses:
// the reading language (add or remove one), and for it the dictionary on the
// card and its first and second online source; below, Keep Wi-Fi on in books.
class DictionariesActivity final : public RowListActivity {
 public:
  DictionariesActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;

 protected:
  void buildRows(std::vector<Row>& rows) override;

 private:
  enum Row_ { Offline = 0, OnlineFirst, OnlineSecond };

  struct CardDictionary {
    std::string name;
    std::string path;
    bool langKnown = false;
    bool sameLanguage = false;  // "en-en": explains words in their own language
    onlinedict::Lang lang = onlinedict::Lang::English;
  };

  std::vector<onlinedict::Lang> chosenLanguages() const;
  onlinedict::Lang current() const;
  bool hasCardDictionary(onlinedict::Lang lang) const;
  std::string offlineName(onlinedict::Lang lang) const;
  void openPicker(int row);
  void openLanguagePicker();
  void openAddLanguage();
  void removeLanguage(int index);
  void toggleWifi();
  void chooseOffline(onlinedict::Lang lang, const std::string& path);

  std::vector<CardDictionary> dictionaries;
  // Each language's chosen dictionary path, read once (render runs often).
  std::string offlinePaths[onlinedict::kLangCount];
  int selected = 0;  // which of the chosen languages the rows below describe
  std::vector<int> pickerValues;  // what each popup row stands for
};
