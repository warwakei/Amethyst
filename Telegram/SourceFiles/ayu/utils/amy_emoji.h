// This is the source code of Amethyst for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
#pragma once

#include <QString>

struct TextWithEntities;

namespace Amethyst {

constexpr auto AmyTagEmojiId = 5228894873519685084ULL;

[[nodiscard]] bool NameHasAmyTag(const QString &name);
[[nodiscard]] TextWithEntities NameWithAmyEmoji(const QString &name);

} // namespace Amethyst
