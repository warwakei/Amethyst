// This is the source code of Amethyst for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
#pragma once

#include "base/timer.h"
#include "boxes/abstract_box.h"

namespace Ui {
class BoxContent;
} // namespace Ui

namespace Passport {

[[nodiscard]] object_ptr<Ui::BoxContent> VerifyEmailBox(
	const QString &email,
	int codeLength,
	Fn<void(QString code)> submit,
	Fn<void()> resend,
	rpl::producer<QString> error,
	rpl::producer<QString> resent);

} // namespace Passport
