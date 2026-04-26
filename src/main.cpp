/*******************************************************************************
 * Copyright (c) 2026 Eclipse Menu Team.
 * Copyright (c) 2026 Nwo5, lyyako.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * https://www.eclipse.org/legal/epl-2.0/.
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *     Eclipse Menu Team - original code for the 'Allow Low Volume' hack
 *     lyyako - adaptation and integration into Better Volume
 *******************************************************************************/

#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/OptionsLayer.hpp>

using namespace geode::prelude;

std::string ftofstr(float num, int decimal) {
	if (decimal == 0) {
		return std::to_string((int)round(num));
	}

	std::ostringstream ss;
	ss << std::fixed << std::setprecision(decimal) << num;

	std::string string{ss.str()};
	string.erase(string.find_last_not_of('0') + 1, std::string::npos);

	if (string.back() == '.') {
		string.pop_back();
	}

	return string;
}

static std::string getVolumeStr(float pVal) {
	auto precision = Mod::get()->getSettingValue<int64_t>("input-precision");
	return ftofstr(pVal * 100, precision);
}

static void setupSlider(bool pIsMusic, CCNode* pLayer, geode::CopyableFunction<void(CCObject*)> pCallback, TextInput*& pInputPtr, Slider*& pSliderPtr) {
	auto slider = typeinfo_cast<Slider*>(pLayer->getChildByIDRecursive(pIsMusic ? "music-slider" : "sfx-slider"));
	if (!slider) {
		return;
	}

	pSliderPtr = slider;
	slider->setZOrder(1);

	auto label = typeinfo_cast<CCLabelBMFont*>(pLayer->getChildByIDRecursive(pIsMusic ? "music-label" : "sfx-label"));
	if (!label) {
		return;
	}

	label->setVisible(false);
	
	std::string labelText{Mod::get()->getSettingValue<std::string>(pIsMusic ? "music-text" : "sfx-text") + ":"};
	auto newLabel = CCLabelBMFont::create(labelText.c_str(), label->getFntFile());
	newLabel->setID(pIsMusic ? "music-label"_spr : "sfx-label"_spr);
	newLabel->setScale(label->getScale());

	auto texture = CCTextureCache::sharedTextureCache()->addImage("sliderBar2.png", true);
	if (texture && Mod::get()->getSettingValue<bool>("colored-bars")) {
		slider->m_sliderBar->setTexture(texture);
		slider->m_sliderBar->setColor(Mod::get()->getSettingValue<ccColor3B>(pIsMusic ? "music-color" : "sfx-color"));
	}

	pInputPtr = TextInput::create(40.0f, "0");
	pInputPtr->setID(pIsMusic ? "music-input"_spr : "sfx-input"_spr);
	pInputPtr->setScale(0.65f);
	pInputPtr->setFilter("0123456789.");
	pInputPtr->setString(getVolumeStr(slider->getValue()));
	pInputPtr->setCallback([=] (const std::string& pStr) {
		if (pStr.empty() || pStr.ends_with('.') || pStr.starts_with('.')) {
			return;
		}

		auto result = utils::numFromString<float>(pStr);
		if (!result) {
			return;
		}

		auto newValue = std::clamp(result.unwrapOr(0.0f), 0.0f, 100.0f) / 100;
		slider->setValue(newValue);
		slider->updateBar();
		pCallback(slider->m_touchLogic->m_thumb);
	});

	auto parent = slider->getParent();
	if (!parent) {
		return;
	}

	auto percentLabel = CCLabelBMFont::create("%", label->getFntFile());
	percentLabel->setScale(label->getScale());

	auto labelMenu = CCMenu::create();
	labelMenu->setID(pIsMusic ? "music-label-menu"_spr : "sfx-label-menu"_spr);
	labelMenu->setPosition(label->getPosition());
	labelMenu->setContentSize(CCSizeZero);
	parent->addChild(labelMenu);

	labelMenu->addChild(newLabel);
	labelMenu->addChild(pInputPtr);
	labelMenu->addChild(percentLabel);
	labelMenu->setLayout(
		RowLayout::create()
			->setAxisAlignment(AxisAlignment::Center)
			->setAutoGrowAxis(0.0f)
			->setGrowCrossAxis(false)
			->setAutoScale(false)
			->setGap(5.0f)
	);

	if (!Mod::get()->getSettingValue<bool>("mute-button")) {
		return;
	}

	auto muteToggle = CCMenuItemExt::createTogglerWithFrameName(
		pIsMusic ? "GJ_musicOffBtn_001.png" : "GJ_fxOffBtn_001.png",
		pIsMusic ? "GJ_musicOnBtn_001.png" : "GJ_fxOnBtn_001.png",
		0.5f,
		[=] (CCMenuItemToggler* pSender) {
			const auto muted = !Mod::get()->getSavedValue<bool>(pIsMusic ? "music-muted" : "sfx-muted");

			if (muted) {
				Mod::get()->setSavedValue<float>(pIsMusic ? "music-volume-ret" : "sfx-volume-ret", slider->getValue());

				slider->setValue(0.0f);
				slider->updateBar();

				pCallback(slider->m_touchLogic->m_thumb);
			} else {
				slider->setValue(
					Mod::get()->getSavedValue<float>(pIsMusic ? "music-volume-ret" : "sfx-volume-ret")
				);
				slider->updateBar();
				
				pCallback(slider->m_touchLogic->m_thumb);

				pSender->toggle(true);
			}
			
			Mod::get()->setSavedValue<bool>(pIsMusic ? "music-muted" : "sfx-muted", muted);
		}
	);
	muteToggle->setID(pIsMusic ? "music-mute-toggle"_spr : "sfx-mute-toggle"_spr);
	muteToggle->toggle(Mod::get()->getSavedValue<bool>(pIsMusic ? "music-muted" : "sfx-muted"));
	muteToggle->setPosition(CCPointZero);

	auto muteButtonMenu = CCMenu::create();
	muteButtonMenu->setID(pIsMusic ? "music-mute-menu"_spr : "sfx-mute-menu"_spr);
	muteButtonMenu->setContentSize(muteToggle->getScaledContentSize());
	muteButtonMenu->addChild(muteToggle);
	parent->addChild(muteButtonMenu);

	if (Mod::get()->getSettingValue<bool>("mute-button-on-right")) {
		muteButtonMenu->setPosition(
			slider->getPositionX() + (slider->m_groove->getScaledContentWidth() + 10.0f) / 2,
			slider->getPositionY()
		);
		slider->setPosition(
			slider->getPositionX() - (muteButtonMenu->getScaledContentWidth() + 10.0f) / 2,
			slider->getPositionY()
		);
	} else {
		muteButtonMenu->setPosition(
			labelMenu->getPositionX() + (slider->m_groove->getScaledContentWidth() * slider->getScale()) / 2,
			labelMenu->getPositionY()
		);
	}
}

static void tryUpdateMuteButton(CCLayer* pLayer, bool pIsMusic) {
	if (!Mod::get()->getSavedValue<bool>(pIsMusic ? "music-muted" : "sfx-muted")) {
		return;
	}

	auto toggler = typeinfo_cast<CCMenuItemToggler*>(
		pLayer->getChildByIDRecursive(pIsMusic ? "music-mute-toggle"_spr : "sfx-mute-toggle"_spr)
	);
	if (!toggler) {
		return;
	}

	Mod::get()->setSavedValue<bool>(pIsMusic ? "music-muted" : "sfx-muted", false);

	toggler->toggle(false);
}

class $modify(PauseLayer) {
	struct Fields {
		TextInput* m_musicInput = nullptr;
		TextInput* m_sfxInput = nullptr;
		Slider* m_musicSlider = nullptr;
		Slider* m_sfxSlider = nullptr;
	};

	void customSetup() {
		PauseLayer::customSetup();

		if (!Mod::get()->getSettingValue<bool>("enable-mod")) {
			return;
		}

		bool swapped = Mod::get()->getSettingValue<bool>("swap-sliders");

		if (swapped) {
			auto musicSlider = this->getChildByIDRecursive("music-slider");
			auto sfxSlider = this->getChildByIDRecursive("sfx-slider");
			auto musicLabel = this->getChildByIDRecursive("music-label");
			auto sfxLabel = this->getChildByIDRecursive("sfx-label");

			if (musicSlider && sfxSlider && musicLabel && sfxLabel) {
				auto musicSliderPos = musicSlider->getPosition();
				auto sfxSliderPos = sfxSlider->getPosition();
				auto musicLabelPos = musicLabel->getPosition();
				auto sfxLabelPos = sfxLabel->getPosition();

				musicSlider->setPosition(sfxSliderPos);
				sfxSlider->setPosition(musicSliderPos);
				musicLabel->setPosition(sfxLabelPos);
				sfxLabel->setPosition(musicLabelPos);
			}
		}
		
		const float HORIZONTAL_SHIFT = 12.0f;
		float musicShift = swapped ? HORIZONTAL_SHIFT : -HORIZONTAL_SHIFT;
		float sfxShift = swapped ? -HORIZONTAL_SHIFT : HORIZONTAL_SHIFT;

		if (auto musicSliderNode = this->getChildByIDRecursive("music-slider")) {
			musicSliderNode->setPositionX(musicSliderNode->getPositionX() + musicShift);
		}
		if (auto musicLabelNode = this->getChildByIDRecursive("music-label")) {
			musicLabelNode->setPositionX(musicLabelNode->getPositionX() + musicShift);
		}

		if (auto sfxSliderNode = this->getChildByIDRecursive("sfx-slider")) {
			sfxSliderNode->setPositionX(sfxSliderNode->getPositionX() + sfxShift);
		}
		if (auto sfxLabelNode = this->getChildByIDRecursive("sfx-label")) {
			sfxLabelNode->setPositionX(sfxLabelNode->getPositionX() + sfxShift);
		}
		
		auto fields = m_fields.self();
		setupSlider(true, this, [this] (CCObject* pSender) { musicSliderChanged(pSender); }, fields->m_musicInput, fields->m_musicSlider);
		setupSlider(false, this, [this] (CCObject* pSender) { sfxSliderChanged(pSender); }, fields->m_sfxInput, fields->m_sfxSlider);
	}

	void musicSliderChanged(CCObject* pSender) {
		auto slider = typeinfo_cast<SliderThumb*>(pSender);
		if (!slider) {
			return;
		}

		auto value = slider->getValue();
		GameManager::sharedState()->m_bgVolume = value;
		FMODAudioEngine::sharedEngine()->setBackgroundMusicVolume(value);

		auto fields = m_fields.self();
		if (fields->m_musicInput && fields->m_musicSlider) {
			fields->m_musicInput->setString(getVolumeStr(fields->m_musicSlider->getValue()));
			tryUpdateMuteButton(this, true);
		}
	}

#if !defined(GEODE_IS_WINDOWS)
	void sfxSliderChanged(CCObject* pSender) {
		auto slider = typeinfo_cast<SliderThumb*>(pSender);
		if (!slider) {
			return;
		}

		auto value = slider->getValue();
		GameManager::sharedState()->m_sfxVolume = value;
		FMODAudioEngine::sharedEngine()->setEffectsVolume(value);

		auto fields = m_fields.self();
		if (fields->m_sfxInput && fields->m_sfxSlider) {
			fields->m_sfxInput->setString(getVolumeStr(fields->m_sfxSlider->getValue()));
			tryUpdateMuteButton(this, false);
		}
	}
#endif
};

class $modify(OptionsLayer) {
	struct Fields {
		TextInput* m_musicInput = nullptr;
		TextInput* m_sfxInput = nullptr;
		Slider* m_musicSlider = nullptr;
		Slider* m_sfxSlider = nullptr;
	};

	void customSetup() {
		OptionsLayer::customSetup();

		if (!Mod::get()->getSettingValue<bool>("enable-mod")) {
			return;
		}

		if (Mod::get()->getSettingValue<bool>("swap-sliders")) {
			auto musicSlider = this->getChildByIDRecursive("music-slider");
			auto sfxSlider = this->getChildByIDRecursive("sfx-slider");
			auto musicLabel = this->getChildByIDRecursive("music-label");
			auto sfxLabel = this->getChildByIDRecursive("sfx-label");

			if (musicSlider && sfxSlider && musicLabel && sfxLabel) {
				auto musicSliderPos = musicSlider->getPosition();
				auto sfxSliderPos = sfxSlider->getPosition();
				auto musicLabelPos = musicLabel->getPosition();
				auto sfxLabelPos = sfxLabel->getPosition();

				musicSlider->setPosition(sfxSliderPos);
				sfxSlider->setPosition(musicSliderPos);
				musicLabel->setPosition(sfxLabelPos);
				sfxLabel->setPosition(musicLabelPos);
			}
		}

		auto fields = m_fields.self();
		setupSlider(true, this, [this] (CCObject* pSender) { musicSliderChanged(pSender); }, fields->m_musicInput, fields->m_musicSlider);
		setupSlider(false, this, [this] (CCObject* pSender) { sfxSliderChanged(pSender); }, fields->m_sfxInput, fields->m_sfxSlider);
	}

	void musicSliderChanged(CCObject* pSender) {
		auto slider = typeinfo_cast<SliderThumb*>(pSender);
		if (!slider) {
			return;
		}

		auto value = slider->getValue();
		GameManager::sharedState()->m_bgVolume = value;
		FMODAudioEngine::sharedEngine()->setBackgroundMusicVolume(value);
		
		if (auto text_input = static_cast<TextInput*>(this->getChildByIDRecursive("music-input"_spr))) {
			text_input->setString(getVolumeStr(value));
		}
		tryUpdateMuteButton(this, true);
	}

	void sfxSliderChanged(CCObject* pSender) {
		auto slider = typeinfo_cast<SliderThumb*>(pSender);
		if (!slider) {
			return;
		}

		auto value = slider->getValue();
		GameManager::sharedState()->m_sfxVolume = value;
		FMODAudioEngine::sharedEngine()->setEffectsVolume(value);
		
		if (auto text_input = static_cast<TextInput*>(this->getChildByIDRecursive("sfx-input"_spr))) {
			text_input->setString(getVolumeStr(value));
		}
		tryUpdateMuteButton(this, false);
	}
};