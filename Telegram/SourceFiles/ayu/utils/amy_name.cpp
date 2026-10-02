// This is the source code of Amethyst for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
#include "ayu/utils/amy_name.h"

#include "apiwrap.h"
#include "base/weak_ptr.h"
#include "data/data_peer_id.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "main/main_session.h"

#include <unordered_set>

namespace Amethyst {
namespace {

std::unordered_set<uint64> EnforceInFlight;

} // namespace

void EnforcePrefixOnSelf(UserData *user) {
	if (!user || !user->isSelf()) {
		return;
	}
	if (HasAmyPrefix(user->firstName)) {
		return;
	}
	if (StripAmyPrefix(user->firstName).isEmpty()) {
		return;
	}
	const auto id = peerToUser(user->id).bare;
	if (!EnforceInFlight.insert(id).second) {
		return;
	}
	const auto full = WithAmyPrefix(user->firstName);
	const auto weak = base::make_weak(&user->session());
	user->session().api().request(MTPaccount_UpdateProfile(
		MTP_flags(MTPaccount_UpdateProfile::Flag::f_first_name),
		MTP_string(full),
		MTPstring(),
		MTPstring()
	)).done([=](const MTPUser &updated) {
		EnforceInFlight.erase(id);
		if (const auto strong = weak.get()) {
			strong->data().processUser(updated);
		}
	}).fail([=](const MTP::Error &) {
		EnforceInFlight.erase(id);
	}).send();
}

} // namespace Amethyst
