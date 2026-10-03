// This is the source code of Amethyst for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
#pragma once

#include <QString>

namespace Main {
class Session;
} // namespace Main

namespace Amethyst {

[[nodiscard]] const QString &DefaultThemeSlug();
void ApplyDefaultThemeOnce(Main::Session *session);

} // namespace Amethyst
