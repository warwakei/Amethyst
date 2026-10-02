// This is the source code of Amethyst for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
#pragma once

#include "settings/settings_common.h"
#include "settings/settings_common_session.h"

namespace Window {
class SessionController;
} // namespace Window

namespace Settings {

class AyuModeration : public Section<AyuModeration> {
public:
	AyuModeration(QWidget *parent, not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

[[nodiscard]] Type AyuModerationId();

} // namespace Settings
