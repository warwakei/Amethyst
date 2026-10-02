// This is the source code of Amethyst for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
#pragma once

#include "base/timer.h"
#include "boxes/abstract_box.h"

namespace Ui {
class InputField;
}

class IrisPunishBox : public Ui::BoxContent {
public:
	IrisPunishBox(
		QWidget*,
		rpl::producer<QString> title,
		rpl::producer<QString> timePlaceholder,
		rpl::producer<QString> reasonPlaceholder,
		Fn<void(QString time, QString reason)> doneCallback);

protected:
	void setInnerFocus() override;
	void prepare() override;
	void resizeEvent(QResizeEvent *e) override;

private:
	void save();

	rpl::producer<QString> _title;
	rpl::producer<QString> _timePlaceholder;
	rpl::producer<QString> _reasonPlaceholder;
	Fn<void(QString time, QString reason)> _doneCallback;

	object_ptr<Ui::InputField> _time;
	object_ptr<Ui::InputField> _reason;

};
