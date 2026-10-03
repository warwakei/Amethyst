// This is the source code of Amethyst for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
#include "boxes/verify_email_box.h"

#include "core/file_utilities.h"
#include "lang/lang_keys.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_passport.h"
#include "ui/text/format_values.h"
#include "ui/widgets/box_content_divider.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/fade_wrap.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/fields/special_fields.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/sent_code_field.h"
#include "ui/widgets/shadow.h"
#include "ui/wrap/fade_wrap.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/wrap/vertical_layout.h"

namespace Passport {
namespace {

class VerifyBox : public Ui::BoxContent {
public:
	VerifyBox(
		QWidget*,
		rpl::producer<QString> title,
		const QString &text,
		int codeLength,
		const QString &openUrl,
		Fn<void(QString code)> submit,
		Fn<void()> resend,
		rpl::producer<QString> call,
		rpl::producer<QString> error,
		rpl::producer<QString> resent);

	void setInnerFocus() override;

protected:
	void prepare() override;

private:
	void setupControls(
		const QString &text,
		int codeLength,
		const QString &openUrl,
		Fn<void(QString code)> submit,
		Fn<void()> resend,
		rpl::producer<QString> call,
		rpl::producer<QString> error,
		rpl::producer<QString> resent);

	rpl::producer<QString> _title;
	Fn<void()> _submit;
	QPointer<Ui::SentCodeField> _code;
	QPointer<Ui::VerticalLayout> _content;

};

VerifyBox::VerifyBox(
	QWidget*,
	rpl::producer<QString> title,
	const QString &text,
	int codeLength,
	const QString &openUrl,
	Fn<void(QString code)> submit,
	Fn<void()> resend,
	rpl::producer<QString> call,
	rpl::producer<QString> error,
	rpl::producer<QString> resent)
: _title(std::move(title)) {
	setupControls(
		text,
		codeLength,
		openUrl,
		submit,
		resend,
		std::move(call),
		std::move(error),
		std::move(resent));
}

void VerifyBox::setupControls(
		const QString &text,
		int codeLength,
		const QString &openUrl,
		Fn<void(QString code)> submit,
		Fn<void()> resend,
		rpl::producer<QString> call,
		rpl::producer<QString> error,
		rpl::producer<QString> resent) {
	_content = Ui::CreateChild<Ui::VerticalLayout>(this);

	const auto small = style::margins(
		st::boxPadding.left(),
		0,
		st::boxPadding.right(),
		st::boxPadding.bottom());
	_content->add(
		object_ptr<Ui::FlatLabel>(
			_content,
			text,
			st::boxLabel),
		small);
	_code = _content->add(
		object_ptr<Ui::SentCodeField>(
			_content,
			st::defaultInputField,
			tr::lng_change_phone_code_title()),
		small);

	const auto problem = _content->add(
		object_ptr<Ui::FadeWrap<Ui::FlatLabel>>(
			_content,
			object_ptr<Ui::FlatLabel>(
				_content,
				QString(),
				st::passportVerifyErrorLabel)),
		small);
	_content->add(
		object_ptr<Ui::FlatLabel>(
			_content,
			std::move(call),
			st::boxDividerLabel),
		small);
	if (!openUrl.isEmpty()) {
		const auto button = _content->add(
			object_ptr<Ui::RoundButton>(
				_content,
				tr::lng_intro_fragment_button(),
				st::fragmentBoxButton),
			small);
		_content->widthValue(
		) | rpl::on_next([=](int w) {
			button->setFullWidth(w - small.left() - small.right());
		}, button->lifetime());
		button->setClickedCallback([=] { ::File::OpenUrl(openUrl); });
	}
	if (resend) {
		auto link = TextWithEntities{ tr::lng_cloud_password_resend(tr::now) };
		link.entities.push_back({
			EntityType::CustomUrl,
			0,
			int(link.text.size()),
			QString("internal:resend") });
		const auto label = _content->add(
			object_ptr<Ui::FlatLabel>(
				_content,
				rpl::single(
					link
				) | rpl::then(rpl::duplicate(
					resent
				) | rpl::map(TextWithEntities::Simple)),
				st::boxDividerLabel),
			small);
		std::move(
			resent
		) | rpl::on_next([=] {
			_content->resizeToWidth(st::boxWidth);
		}, _content->lifetime());
		label->overrideLinkClickHandler(resend);
	}
	std::move(
		error
	) | rpl::on_next([=](const QString &error) {
		if (error.isEmpty()) {
			problem->hide(anim::type::normal);
		} else {
			problem->entity()->setText(error);
			_content->resizeToWidth(st::boxWidth);
			problem->show(anim::type::normal);
			_code->showError();
		}
	}, lifetime());

	_submit = [=] {
		submit(_code->getDigitsOnly());
	};
	if (codeLength > 0) {
		_code->setAutoSubmit(codeLength, _submit);
	} else {
		_code->submits() | rpl::on_next(_submit, _code->lifetime());
	}
	_code->changes(
	) | rpl::on_next([=] {
		problem->hide(anim::type::normal);
	}, _code->lifetime());
}

void VerifyBox::setInnerFocus() {
	_code->setFocusFast();
}

void VerifyBox::prepare() {
	setTitle(std::move(_title));

	addButton(tr::lng_change_phone_new_submit(), _submit);
	addButton(tr::lng_cancel(), [=] { closeBox(); });

	_content->resizeToWidth(st::boxWidth);
	_content->heightValue(
	) | rpl::on_next([=](int height) {
		setDimensions(st::boxWidth, height);
	}, _content->lifetime());
}

object_ptr<Ui::BoxContent> VerifyEmailBox(
		const QString &email,
		int codeLength,
		Fn<void(QString code)> submit,
		Fn<void()> resend,
		rpl::producer<QString> error,
		rpl::producer<QString> resent) {
	return Box<VerifyBox>(
		tr::lng_passport_email_title(),
		tr::lng_passport_confirm_email(tr::now, lt_email, email),
		codeLength,
		QString(),
		submit,
		resend,
		rpl::single(QString()),
		std::move(error),
		std::move(resent));
}

} // namespace Passport
