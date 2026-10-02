// This is the source code of Amethyst for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
#include "ayu/ui/settings/settings_moderation.h"

#include "lang_auto.h"
#include "ayu/ayu_settings.h"
#include "ayu/ui/settings/ayu_builder.h"
#include "ayu/ui/settings/settings_ayu_utils.h"
#include "ayu/ui/settings/settings_main.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

namespace Settings {

using namespace Builder;
using namespace AyuBuilder;

namespace {

void BuildIrisHelper(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	builder.addSubsectionTitle(tr::ayu_CategoryModeration());

	ayu.addSettingToggle({
		.id = u"ayu/irisHelper"_q,
		.title = tr::ayu_IrisHelper(),
		.getter = &AyuSettings::irisHelper,
		.setter = &AyuSettings::setIrisHelper,
		.icon = { &st::menuIconPermissions },
	});
	builder.addSkip();
	builder.addDividerText(tr::ayu_IrisHelperDescription());
	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = AyuModeration::Id(),
	.parentId = AyuMain::Id(),
	.title = &tr::ayu_CategoryModeration,
	.icon = &st::menuIconPermissions,
}, [](SectionBuilder &builder) {
	auto ayu = AyuSectionBuilder(builder);

	builder.addSkip();
	BuildIrisHelper(builder, ayu);
});

} // namespace

rpl::producer<QString> AyuModeration::title() {
	return tr::ayu_CategoryModeration();
}

AyuModeration::AyuModeration(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void AyuModeration::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type AyuModerationId() {
	return AyuModeration::Id();
}

} // namespace Settings
