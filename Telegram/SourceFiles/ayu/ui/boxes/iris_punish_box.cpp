// This is the source code of Amethyst for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
#include "ayu/ui/boxes/iris_punish_box.h"

#include "lang_auto.h"
#include "styles/style_add_contact_box.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"
#include "ui/widgets/fields/input_field.h"

#include <utility>

IrisPunishBox::IrisPunishBox(
	QWidget*,
	rpl::producer<QString> title,
	rpl::producer<QString> timePlaceholder,
	rpl::producer<QString> reasonPlaceholder,
	Fn<void(QString time, QString reason)> doneCallback)
: _title(std::move(title))
, _timePlaceholder(std::move(timePlaceholder))
, _reasonPlaceholder(std::move(reasonPlaceholder))
, _doneCallback(std::move(doneCallback))
, _time(
	this,
	st::defaultInputField,
	_timePlaceholder,
	QString())
, _reason(
	this,
	st::defaultInputField,
	_reasonPlaceholder,
	QString()) {
}

void IrisPunishBox::prepare() {
	setTitle(_title);

	const auto fieldWidth = st::boxWidth
		- st::boxPadding.left()
		- st::newGroupInfoPadding.left()
		- st::boxPadding.right();
	_time->resize(fieldWidth, _time->height());
	_reason->resize(fieldWidth, _reason->height());

	auto newHeight = st::contactPadding.top()
		+ _time->height()
		+ st::contactSkip
		+ _reason->height();
	newHeight += st::boxPadding.bottom() + st::contactPadding.bottom();
	setDimensions(st::boxWidth, newHeight);

	addButton(tr::ayu_IrisPunishDone(), [=] {
		save();
	});
	addButton(tr::lng_cancel(), [=] {
		closeBox();
	});

	_time->submits(
	) | rpl::on_next([=] {
		_reason->setFocusFast();
	}, _time->lifetime());
	_reason->submits(
	) | rpl::on_next([=] {
		save();
	}, _reason->lifetime());
}

void IrisPunishBox::setInnerFocus() {
	_time->setFocusFast();
}

void IrisPunishBox::resizeEvent(QResizeEvent *e) {
	BoxContent::resizeEvent(e);

	const auto width = _time->width();
	_time->resize(width, _time->height());
	_reason->resize(width, _reason->height());

	const auto left = st::boxPadding.left() + st::newGroupInfoPadding.left();
	auto top = st::contactPadding.top();
	_time->moveToLeft(left, top);
	top += _time->height() + st::contactSkip;
	_reason->moveToLeft(left, top);
}

void IrisPunishBox::save() {
	_doneCallback(
		_time->getLastText().trimmed(),
		_reason->getLastText().trimmed());
	closeBox();
}
