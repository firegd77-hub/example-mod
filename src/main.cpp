#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/file.hpp>
#include <filesystem>
#include <map>

using namespace geode::prelude;

namespace {
    std::string setStat(std::string xml, std::string const& key, int value) {
        auto gs = xml.find("<k>GS_value</k>");
        if (gs == std::string::npos) return xml;

        auto tag = "<k>" + key + "</k><s>";
        auto val = std::to_string(value);
        auto p = xml.find(tag, gs);

        if (p != std::string::npos) {
            auto s = p + tag.size();
            auto e = xml.find("</s>", s);
            xml.replace(s, e - s, val);
        } else {
            auto d = xml.find("<d>", gs);
            xml.insert(d + 3, tag + val + "</s>");
        }
        return xml;
    }

    bool editRawSave(std::string const& name, std::map<std::string, int> const& stats) {
        std::filesystem::path dir = CCFileUtils::get()->getWritablePath().c_str();
        auto path = dir / name;

        auto raw = file::readString(path);
        if (!raw) return false;

        std::error_code ec;
        std::filesystem::copy_file(
            path, path.string() + ".bak",
            std::filesystem::copy_options::overwrite_existing, ec
        );

        std::string xml = ZipUtils::decompressString(raw.unwrap(), true, 11).c_str();
        if (xml.empty()) return false;

        for (auto const& [k, v] : stats) xml = setStat(xml, k, v);

        std::string out = ZipUtils::compressString(xml, true, 11).c_str();
        return file::writeString(path, out).isOk();
    }
}

class $modify(SaveEditMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Edit Save"),
            this,
            menu_selector(SaveEditMenu::onEditSave)
        );

        if (auto menu = this->getChildByID("bottom-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        }
        return true;
    }

    void onEditSave(CCObject*) {
        bool ok = editRawSave("CCGameManager2.dat", {
            {"6", 1000}, {"13", 500}, {"14", 5000}
        });

        if (!ok) {
            FLAlertLayer::create("Error", "Couldn't edit the save file.", "OK")->show();
            return;
        }

        createQuickPopup(
            "Save Edited",
            "Edit written. The game must close without saving for it to stick.",
            "Later", "Quit now",
            [](auto, bool quit) { if (quit) std::_Exit(0); }
        );
    }
};
