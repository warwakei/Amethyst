// This is the source code of Amethyst for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
#include "ayu/utils/amy_theme.h"

#include "apiwrap.h"
#include "ayu/ayu_settings.h"
#include "base/weak_ptr.h"
#include "data/data_cloud_themes.h"
#include "data/data_session.h"
#include "main/main_session.h"

namespace Amethyst {

const QString &DefaultThemeSlug() {
	static const auto slug = QString("iwakewar");
	return slug;
}

void ApplyDefaultThemeOnce(Main::Session *session) {
	if (!session) {
		return;
	}
	auto &settings = AyuSettings::getInstance();
	if (settings.defaultThemeApplied()) {
		return;
	}
	const auto weak = base::make_weak(session);
	session->api().request(MTPaccount_GetTheme(
		MTP_string(Data::CloudThemes::Format()),
		MTP_inputThemeSlug(MTP_string(DefaultThemeSlug()))
	)).done([=](const MTPTheme &result) {
		if (const auto strong = weak.get()) {
			const auto cloud = Data::CloudTheme::Parse(
				not_null<Main::Session*>(strong),
				result,
				true);
			if (cloud.documentId) {
				strong->data().cloudThemes().applyFromDocument(cloud);
			}
			AyuSettings::getInstance().setDefaultThemeApplied(true);
		}
	}).fail([=](const MTP::Error &) {
	}).send();
}

} // namespace Amethyst
