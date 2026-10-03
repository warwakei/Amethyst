/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/sections/settings_advanced.h"

#include "settings/settings_common_session.h"

#include "api/api_global_privacy.h"
#include "apiwrap.h"
#include "base/call_delayed.h"
#include "base/platform/base_platform_custom_app_icon.h"
#include "base/platform/base_platform_info.h"
#include "base/screen_reader_state.h"
#include "boxes/about_box.h"
#include "boxes/auto_download_box.h"
#include "boxes/connection_box.h"
#include "boxes/download_path_box.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "core/file_utilities.h"
#include "core/launcher.h"
#include "core/update_checker.h"
#include "data/data_auto_download.h"
#include "data/data_session.h"
#include "export/export_manager.h"
#include "info/downloads/info_downloads_widget.h"
#include "info/info_memento.h"
#include "lang/lang_keys.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "main/main_session_settings.h"
#include "mtproto/facade.h"
#include "mtproto/mtp_instance.h"
#include "platform/platform_specific.h"
#include "settings/settings_builder.h"
#include "settings/sections/settings_local_storage.h"
#include "settings/sections/settings_main.h"
#include "settings/sections/settings_chat.h"
#include "settings/settings_power_saving.h"
#include "settings/sections/settings_privacy_security.h"
#include "storage/localstorage.h"
#include "storage/storage_domain.h"
#include "tray.h"
#include "ui/boxes/confirm_box.h"
#include "ui/boxes/single_choice_box.h"
#include "ui/gl/gl_detection.h"
#include "ui/layers/generic_box.h"
#include "ui/painter.h"
#include "ui/platform/ui_platform_window.h"
#include "ui/power_saving.h"
#include "ui/rp_widget.h"
#include "ui/screen_reader_mode.h"
#include "ui/text/format_values.h"
#include "ui/ui_utility.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_controller.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

#ifdef Q_OS_MAC
#include "base/platform/mac/base_confirm_quit.h"
#endif // Q_OS_MAC

#ifndef TDESKTOP_DISABLE_SPELLCHECK
#include "boxes/dictionaries_manager.h"
#include "chat_helpers/spellchecker_common.h"
#include "spellcheck/platform/platform_spellcheck.h"
#endif // !TDESKTOP_DISABLE_SPELLCHECK

#include <ksandbox.h>

namespace Settings {
namespace {

using namespace Builder;

#if defined Q_OS_MAC && !defined OS_MAC_STORE
[[nodiscard]] const QImage &IconMacRound() {
	static const auto result = QImage(u":/gui/art/icon_round512@2x.png"_q);
	return result;
}
#endif // Q_OS_MAC && !OS_MAC_STORE

void BuildDataStorageSection(SectionBuilder &builder) {
	const auto controller = builder.controller();
	const auto container = builder.container();
	const auto session = builder.session();
	const auto account = &session->account();

	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"advanced/data_storage"_q,
		.title = tr::lng_settings_data_storage(),
		.keywords = { u"storage"_q, u"data"_q, u"download"_q, u"connection"_q },
	});

	const auto connectionType = [=] {
		const auto transport = account->mtp().dctransport();
		if (!Core::App().settings().proxy().isEnabled()) {
			return transport.isEmpty()
				? tr::lng_connection_auto_connecting(tr::now)
				: tr::lng_connection_auto(tr::now, lt_transport, transport);
		} else {
			return transport.isEmpty()
				? tr::lng_connection_proxy_connecting(tr::now)
				: tr::lng_connection_proxy(tr::now, lt_transport, transport);
		}
	};

	builder.addButton({
		.id = u"advanced/connection_type"_q,
		.title = tr::lng_settings_connection_type(),
		.icon = { &st::menuIconNetwork },
		.label = rpl::merge(
			Core::App().settings().proxy().connectionTypeChanges(),
			tr::lng_connection_auto_connecting() | rpl::to_empty
		) | rpl::map(connectionType),
		.onClick = [=] {
			controller->window().show(
				ProxiesBoxController::CreateOwningBox(account));
		},
		.keywords = { u"connection"_q, u"proxy"_q, u"network"_q, u"vpn"_q },
	});

	const auto showDownloadPath = container
		? container->lifetime().make_state<rpl::variable<bool>>(
			!Core::App().settings().askDownloadPath())
		: nullptr;

	auto downloadLabel = Core::App().settings().downloadPathValue(
	) | rpl::map([](const QString &text) {
		if (text.isEmpty()) {
			return Core::App().canReadDefaultDownloadPath()
				? tr::lng_download_path_default(tr::now)
				: tr::lng_download_path_temp(tr::now);
		} else if (text == FileDialog::Tmp()) {
			return tr::lng_download_path_temp(tr::now);
		}
		return QDir::toNativeSeparators(text);
	});

	builder.addButton({
		.id = u"advanced/download_path"_q,
		.title = tr::lng_download_path(),
		.icon = { &st::menuIconShowInFolder },
		.label = std::move(downloadLabel),
		.onClick = [=] {
			controller->show(Box<DownloadPathBox>(controller));
		},
		.keywords = { u"download"_q, u"path"_q, u"folder"_q },
		.shown = showDownloadPath
			? showDownloadPath->value()
			: rpl::single(true) | rpl::type_erased,
	});

	builder.addButton({
		.id = u"advanced/downloads"_q,
		.title = tr::lng_downloads_section(),
		.icon = { &st::menuIconDownload },
		.onClick = [=] {
			if (controller) {
				controller->showSection(
					Info::Downloads::Make(controller->session().user()));
			}
		},
		.keywords = { u"downloads"_q, u"files"_q },
	});

	const auto askDownloadPath = builder.addButton({
		.id = u"advanced/ask_download"_q,
		.title = tr::lng_download_path_ask(),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(Core::App().settings().askDownloadPath()),
		.keywords = { u"download"_q, u"path"_q, u"ask"_q },
	});

	if (askDownloadPath) {
		askDownloadPath->toggledValue(
		) | rpl::filter([](bool checked) {
			return (checked != Core::App().settings().askDownloadPath());
		}) | rpl::on_next([=](bool checked) {
			Core::App().settings().setAskDownloadPath(checked);
			Core::App().saveSettingsDelayed();
			if (showDownloadPath) {
				*showDownloadPath = !checked;
			}
		}, askDownloadPath->lifetime());
	}

	builder.addSkip(st::settingsCheckboxesSkip);
}

void BuildAutoDownloadSection(SectionBuilder &builder) {
	const auto controller = builder.controller();
	const auto container = builder.container();
	const auto session = builder.session();
	builder.addDivider();
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"advanced/auto_download"_q,
		.title = tr::lng_media_auto_settings(),
		.keywords = { u"auto"_q, u"download"_q, u"media"_q },
	});

	using Source = Data::AutoDownload::Source;

	struct State {
		rpl::event_stream<> changes;
	};
	const auto state = container
		? container->lifetime().make_state<State>()
		: nullptr;
	const auto shouldBeChecked = [=](Source source) {
		return HasEnabledTypes(session->settings().autoDownload(), source);
	};
	const auto add = [&](
			QString id,
			rpl::producer<QString> title,
			const style::icon *icon,
			Source source,
			QStringList keywords) {
		const auto row = builder.addButton({
			.id = std::move(id),
			.title = std::move(title),
			.icon = { icon },
			.onClick = [=] {
				auto box = Box<AutoDownloadBox>(session, source);
				box->boxClosing() | rpl::on_next(crl::guard(container, [=] {
					state->changes.fire({});
				}), box->lifetime());
				controller->show(std::move(box));
			},
			.keywords = std::move(keywords),
		});
		if (!row) {
			return;
		}
		const auto [toggle, checkView] = AddSeparatedToggle(
			row,
			st::settingsButton,
			shouldBeChecked(source));
		state->changes.events() | rpl::on_next([=] {
			checkView->setChecked(shouldBeChecked(source), anim::type::normal);
		}, row->lifetime());
		toggle->clicks() | rpl::on_next([=] {
			auto &data = session->settings().autoDownload();
			const auto enable = !checkView->checked();
			if (enable) {
				SetDefaultsForSource(data, source);
				session->data().photoLoadSettingsChanged();
				session->data().documentLoadSettingsChanged();
			} else {
				SetDisabledForSource(data, source);
				session->data().checkPlayingAnimations();
			}
			session->saveSettingsDelayed();
			state->changes.fire({});
		}, toggle->lifetime());
	};

	add(
		u"advanced/auto_download_private"_q,
		tr::lng_media_auto_in_private(),
		&st::menuIconProfile,
		Source::User,
		{ u"auto"_q, u"download"_q, u"private"_q, u"media"_q });
	add(
		u"advanced/auto_download_groups"_q,
		tr::lng_media_auto_in_groups(),
		&st::menuIconGroups,
		Source::Group,
		{ u"auto"_q, u"download"_q, u"groups"_q, u"media"_q });
	add(
		u"advanced/auto_download_channels"_q,
		tr::lng_media_auto_in_channels(),
		&st::menuIconChannel,
		Source::Channel,
		{ u"auto"_q, u"download"_q, u"channels"_q, u"media"_q });

	builder.addSkip(st::settingsCheckboxesSkip);
}

class Advanced : public Section<Advanced> {
public:
	Advanced(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

const auto kMeta = BuildHelper({
	.id = Advanced::Id(),
	.parentId = MainId(),
	.title = &tr::lng_settings_advanced,
	.icon = &st::menuIconManage,
}, [](SectionBuilder &builder) {
	BuildDataStorageSection(builder);
	BuildAutoDownloadSection(builder);
});

const SectionBuildMethod kAdvancedSection = kMeta.build;

Advanced::Advanced(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> Advanced::title() {
	return tr::lng_settings_advanced();
}

void Advanced::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	build(content, kAdvancedSection);

	Ui::ResizeFitChild(this, content);
}

} // namespace

void SetupConnectionType(
		not_null<Window::Controller*> controller,
		not_null<::Main::Account*> account,
		not_null<Ui::VerticalLayout*> container) {
	const auto connectionType = [=] {
		const auto transport = account->mtp().dctransport();
		if (!Core::App().settings().proxy().isEnabled()) {
			return transport.isEmpty()
				? tr::lng_connection_auto_connecting(tr::now)
				: tr::lng_connection_auto(tr::now, lt_transport, transport);
		} else {
			return transport.isEmpty()
				? tr::lng_connection_proxy_connecting(tr::now)
				: tr::lng_connection_proxy(tr::now, lt_transport, transport);
		}
	};
	const auto button = AddButtonWithLabel(
		container,
		tr::lng_settings_connection_type(),
		rpl::merge(
			Core::App().settings().proxy().connectionTypeChanges(),
			tr::lng_connection_auto_connecting() | rpl::to_empty
		) | rpl::map([=] { return connectionType(); }),
		st::settingsButton,
		{ &st::menuIconNetwork });
	button->addClickHandler([=] {
		controller->show(ProxiesBoxController::CreateOwningBox(account));
	});
}

bool HasUpdate() {
	return !Core::UpdaterDisabled();
}

void SetupUpdate(not_null<Ui::VerticalLayout*> container) {
	if (!HasUpdate()) {
		return;
	}

	const auto texts = Ui::CreateChild<rpl::event_stream<QString>>(
		container.get());
	const auto downloading = Ui::CreateChild<rpl::event_stream<bool>>(
		container.get());
	const auto version = tr::lng_settings_current_version(
		tr::now,
		lt_version,
		currentVersionText());
	const auto toggle = container->add(object_ptr<Button>(
		container,
		tr::lng_settings_update_automatically(),
		st::settingsUpdateToggle));
	const auto label = Ui::CreateChild<Ui::FlatLabel>(
		toggle,
		texts->events(),
		st::settingsUpdateState);

	const auto options = container->add(
		object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
			container,
			object_ptr<Ui::VerticalLayout>(container)));
	const auto inner = options->entity();
	const auto install = (cAlphaVersion() || KSandbox::isInside())
		? nullptr
		: inner->add(object_ptr<Button>(
			inner,
			tr::lng_settings_install_beta(),
			st::settingsButtonNoIcon));

	const auto check = inner->add(object_ptr<Button>(
		inner,
		tr::lng_settings_check_now(),
		st::settingsButtonNoIcon));
	const auto update = Ui::CreateChild<Button>(
		check,
		tr::lng_update_telegram(),
		st::settingsUpdate);
	update->hide();
	check->widthValue() | rpl::on_next([=](int width) {
		update->resizeToWidth(width);
		update->moveToLeft(0, 0);
	}, update->lifetime());

	rpl::combine(
		toggle->widthValue(),
		label->widthValue()
	) | rpl::on_next([=] {
		label->moveToLeft(
			st::settingsUpdateStatePosition.x(),
			st::settingsUpdateStatePosition.y());
	}, label->lifetime());
	label->setAttribute(Qt::WA_TransparentForMouseEvents);

	const auto showDownloadProgress = [=](
			int64 ready,
			int64 total,
			bool preferPercent) {
		const auto formatted = [&] {
			if (!preferPercent) {
				return Ui::FormatDownloadText(ready, total);
			}
			const auto percent = (total > 0)
				? std::clamp((ready * 100) / float64(total), 0., 100.)
				: 0.;
			auto result = QString::number(percent, 'f', 2);
			if (result.contains('.')) {
				while (result.endsWith('0')) {
					result.chop(1);
				}
				if (result.endsWith('.')) {
					result.chop(1);
				}
			}
			return result + '%';
		}();
		texts->fire(tr::lng_settings_downloading_update(
			tr::now,
			lt_progress,
			formatted));
		downloading->fire(true);
	};
	const auto setDefaultStatus = [=](const Core::UpdateChecker &checker) {
		using State = Core::UpdateChecker::State;
		const auto state = checker.state();
		switch (state) {
		case State::Download:
			showDownloadProgress(
				checker.already(),
				checker.size(),
				checker.percent());
			break;
		case State::Ready:
			texts->fire(tr::lng_settings_update_ready(tr::now));
			update->show();
			break;
		default:
			texts->fire_copy(version);
			break;
		}
	};

	toggle->toggleOn(rpl::single(cAutoUpdate()));
	toggle->toggledValue(
	) | rpl::filter([](bool toggled) {
		return (toggled != cAutoUpdate());
	}) | rpl::on_next([=](bool toggled) {
		cSetAutoUpdate(toggled);

		Local::writeSettings();
		Core::UpdateChecker checker;
		if (cAutoUpdate()) {
			checker.start();
		} else {
			checker.stop();
			setDefaultStatus(checker);
		}
	}, toggle->lifetime());

	if (install) {
		install->toggleOn(rpl::single(cInstallBetaVersion()));
		install->toggledValue(
		) | rpl::filter([](bool toggled) {
			return (toggled != cInstallBetaVersion());
		}) | rpl::on_next([=](bool toggled) {
			cSetInstallBetaVersion(toggled);
			Core::Launcher::Instance().writeInstallBetaVersionsSetting();

			Core::UpdateChecker checker;
			checker.stop();
			if (toggled) {
				cSetLastUpdateCheck(0);
			}
			checker.start();
		}, toggle->lifetime());
	}

	Core::UpdateChecker checker;
	options->toggleOn(rpl::combine(
		toggle->toggledValue(),
		downloading->events_starting_with(
			checker.state() == Core::UpdateChecker::State::Download)
	) | rpl::map([](bool check, bool downloading) {
		return check && !downloading;
	}));

	checker.checking() | rpl::on_next([=] {
		options->setAttribute(Qt::WA_TransparentForMouseEvents);
		texts->fire(tr::lng_settings_update_checking(tr::now));
		downloading->fire(false);
	}, options->lifetime());
	checker.isLatest() | rpl::on_next([=] {
		options->setAttribute(Qt::WA_TransparentForMouseEvents, false);
		texts->fire(tr::lng_settings_latest_installed(tr::now));
		downloading->fire(false);
	}, options->lifetime());
	checker.progress(
	) | rpl::on_next([=](Core::UpdateChecker::Progress progress) {
		showDownloadProgress(
			progress.already,
			progress.size,
			progress.percent);
	}, options->lifetime());
	checker.failed() | rpl::on_next([=] {
		options->setAttribute(Qt::WA_TransparentForMouseEvents, false);
		texts->fire(tr::lng_settings_update_fail(tr::now));
		downloading->fire(false);
	}, options->lifetime());
	checker.ready() | rpl::on_next([=] {
		options->setAttribute(Qt::WA_TransparentForMouseEvents, false);
		texts->fire(tr::lng_settings_update_ready(tr::now));
		update->show();
		downloading->fire(false);
	}, options->lifetime());

	setDefaultStatus(checker);

	check->addClickHandler([] {
		Core::UpdateChecker checker;

		cSetLastUpdateCheck(0);
		checker.start();
	});
	update->addClickHandler([] {
		if (!Core::UpdaterDisabled()) {
			Core::checkReadyUpdate();
		}
		Core::Restart();
	});
}

void SetupWindowTitleContent(
		Window::SessionController *controller,
		not_null<Ui::VerticalLayout*> container) {
	const auto checkbox = [&](rpl::producer<QString> &&label, bool checked) {
		return object_ptr<Ui::Checkbox>(
			container,
			std::move(label),
			checked,
			st::settingsCheckbox);
	};
	const auto addCheckbox = [&](
			rpl::producer<QString> &&label,
			bool checked) {
		return container->add(
			checkbox(std::move(label), checked),
			st::settingsCheckboxPadding);
	};
	const auto settings = &Core::App().settings();
	if (controller) {
		const auto content = [=] {
			return settings->windowTitleContent();
		};
		const auto showChatName = addCheckbox(
			tr::lng_settings_title_chat_name(),
			!content().hideChatName);
		showChatName->checkedChanges(
		) | rpl::filter([=](bool checked) {
			return (checked == content().hideChatName);
		}) | rpl::on_next([=](bool checked) {
			auto updated = content();
			updated.hideChatName = !checked;
			settings->setWindowTitleContent(updated);
			Core::App().saveSettingsDelayed();
		}, showChatName->lifetime());

		if (Core::App().domain().accountsAuthedCount() > 1) {
			const auto showAccountName = addCheckbox(
				tr::lng_settings_title_account_name(),
				!content().hideAccountName);
			showAccountName->checkedChanges(
			) | rpl::filter([=](bool checked) {
				return (checked == content().hideAccountName);
			}) | rpl::on_next([=](bool checked) {
				auto updated = content();
				updated.hideAccountName = !checked;
				settings->setWindowTitleContent(updated);
				Core::App().saveSettingsDelayed();
			}, showAccountName->lifetime());
		}

		const auto showTotalUnread = addCheckbox(
			tr::lng_settings_title_total_count(),
			!content().hideTotalUnread);
		showTotalUnread->checkedChanges(
		) | rpl::filter([=](bool checked) {
			return (checked == content().hideTotalUnread);
		}) | rpl::on_next([=](bool checked) {
			auto updated = content();
			updated.hideTotalUnread = !checked;
			settings->setWindowTitleContent(updated);
			Core::App().saveSettingsDelayed();
		}, showTotalUnread->lifetime());
	}

	if (Ui::Platform::NativeWindowFrameSupported()) {
		const auto nativeFrame = addCheckbox(
			Platform::IsWayland()
				? tr::lng_settings_qt_frame()
				: tr::lng_settings_native_frame(),
			Core::App().settings().nativeWindowFrame());

		nativeFrame->checkedChanges(
		) | rpl::filter([](bool checked) {
			return (checked != Core::App().settings().nativeWindowFrame());
		}) | rpl::on_next([=](bool checked) {
			Core::App().settings().setNativeWindowFrame(checked);
			Core::App().saveSettingsDelayed();
		}, nativeFrame->lifetime());
	}
}

void SetupSystemIntegrationContent(
		Window::SessionController *controller,
		not_null<Ui::VerticalLayout*> container) {
	using WorkMode = Core::Settings::WorkMode;

	const auto checkbox = [&](rpl::producer<QString> &&label, bool checked) {
		return object_ptr<Ui::Checkbox>(
			container,
			std::move(label),
			checked,
			st::settingsCheckbox);
	};
	const auto addCheckbox = [&](
			rpl::producer<QString> &&label,
			bool checked) {
		return container->add(
			checkbox(std::move(label), checked),
			st::settingsCheckboxPadding);
	};
	const auto addSlidingCheckbox = [&](
			rpl::producer<QString> &&label,
			bool checked) {
		return container->add(
			object_ptr<Ui::SlideWrap<Ui::Checkbox>>(
				container,
				checkbox(std::move(label), checked),
				st::settingsCheckboxPadding));
	};

	const auto settings = &Core::App().settings();
	if (Platform::TrayIconSupported()) {
		const auto trayEnabled = [=] {
			const auto workMode = settings->workMode();
			return (workMode == WorkMode::TrayOnly)
				|| (workMode == WorkMode::WindowAndTray);
		};
		const auto tray = addCheckbox(
			tr::lng_settings_workmode_tray(),
			trayEnabled());
		const auto monochrome = Platform::HasMonochromeSetting()
			? addSlidingCheckbox(
				tr::lng_settings_monochrome_icon(),
				settings->trayIconMonochrome())
			: nullptr;
		if (monochrome) {
			monochrome->toggle(tray->checked(), anim::type::instant);

			monochrome->entity()->checkedChanges(
			) | rpl::filter([=](bool value) {
				return (value != settings->trayIconMonochrome());
			}) | rpl::on_next([=](bool value) {
				settings->setTrayIconMonochrome(value);
				Core::App().saveSettingsDelayed();
			}, monochrome->lifetime());
		}

		const auto taskbarEnabled = [=] {
			const auto workMode = settings->workMode();
			return (workMode == WorkMode::WindowOnly)
				|| (workMode == WorkMode::WindowAndTray);
		};
		const auto taskbar = Platform::SkipTaskbarSupported()
			? addCheckbox(
				tr::lng_settings_workmode_window(),
				taskbarEnabled())
			: nullptr;

		const auto updateWorkmode = [=] {
			const auto newMode = tray->checked()
				? ((!taskbar || taskbar->checked())
					? WorkMode::WindowAndTray
					: WorkMode::TrayOnly)
				: WorkMode::WindowOnly;
			if ((newMode == WorkMode::WindowAndTray
				|| newMode == WorkMode::TrayOnly)
				&& settings->workMode() != newMode) {
				cSetSeenTrayTooltip(false);
			}
			settings->setWorkMode(newMode);
			Core::App().saveSettingsDelayed();
		};

		tray->checkedChanges(
		) | rpl::filter([=](bool checked) {
			return (checked != trayEnabled());
		}) | rpl::on_next([=](bool checked) {
			if (!checked && taskbar && !taskbar->checked()) {
				taskbar->setChecked(true);
			} else {
				updateWorkmode();
			}
			if (monochrome) {
				monochrome->toggle(checked, anim::type::normal);
			}
		}, tray->lifetime());

		if (taskbar) {
			taskbar->checkedChanges(
			) | rpl::filter([=](bool checked) {
				return (checked != taskbarEnabled());
			}) | rpl::on_next([=](bool checked) {
				if (!checked && !tray->checked()) {
					tray->setChecked(true);
				} else {
					updateWorkmode();
				}
			}, taskbar->lifetime());
		}
	}

#ifdef Q_OS_MAC
	const auto warnBeforeQuit = addCheckbox(
		tr::lng_settings_mac_warn_before_quit(
			lt_text,
			rpl::single(Platform::ConfirmQuit::QuitKeysString())),
		settings->macWarnBeforeQuit());
	warnBeforeQuit->checkedChanges(
	) | rpl::filter([=](bool checked) {
		return (checked != settings->macWarnBeforeQuit());
	}) | rpl::on_next([=](bool checked) {
		settings->setMacWarnBeforeQuit(checked);
		Core::App().saveSettingsDelayed();
	}, warnBeforeQuit->lifetime());

#ifndef OS_MAC_STORE
	const auto enabled = [=] {
		const auto digest = base::Platform::CurrentCustomAppIconDigest();
		return digest && (settings->macRoundIconDigest() == digest);
	};
	const auto roundIcon = addCheckbox(
		tr::lng_settings_mac_round_icon(),
		enabled());
	roundIcon->checkedChanges(
	) | rpl::filter([=](bool checked) {
		return (checked != enabled());
	}) | rpl::on_next([=](bool checked) {
		const auto digest = checked
			? base::Platform::SetCustomAppIcon(IconMacRound())
			: std::optional<uint64>();
		if (!checked) {
			base::Platform::ClearCustomAppIcon();
		}
		Window::OverrideApplicationIcon(checked ? IconMacRound() : QImage());
		Core::App().refreshApplicationIcon();
		settings->setMacRoundIconDigest(digest);
		Core::App().saveSettings();
	}, roundIcon->lifetime());
#endif // OS_MAC_STORE
#elif defined Q_OS_WIN // Q_OS_MAC
	using Behavior = Core::Settings::CloseBehavior;
	const auto closeToTaskbar = addSlidingCheckbox(
		tr::lng_settings_close_to_taskbar(),
		settings->closeBehavior() == Behavior::CloseToTaskbar);

	const auto closeToTaskbarShown = std::make_shared<
		rpl::variable<bool>
	>(false);
	settings->workModeValue(
	) | rpl::on_next([=](WorkMode workMode) {
		*closeToTaskbarShown = !Core::App().tray().has();
	}, closeToTaskbar->lifetime());

	closeToTaskbar->toggleOn(closeToTaskbarShown->value());
	closeToTaskbar->entity()->checkedChanges(
	) | rpl::map([=](bool checked) {
		return checked ? Behavior::CloseToTaskbar : Behavior::Quit;
	}) | rpl::filter([=](Behavior value) {
		return (settings->closeBehavior() != value);
	}) | rpl::on_next([=](Behavior value) {
		settings->setCloseBehavior(value);
		Local::writeSettings();
	}, closeToTaskbar->lifetime());
#endif // Q_OS_MAC || Q_OS_WIN

	if (Platform::AutostartSupported() && controller) {
		const auto minimizedToggled = [=] {
			return cStartMinimized()
				&& !controller->session().domain().local().hasLocalPasscode();
		};

		const auto autostart = addCheckbox(
			tr::lng_settings_auto_start(),
			cAutoStart());
		const auto minimized = addSlidingCheckbox(
			tr::lng_settings_start_min(),
			minimizedToggled());

		autostart->checkedChanges(
		) | rpl::filter([](bool checked) {
			return (checked != cAutoStart());
		}) | rpl::on_next([=](bool checked) {
			const auto weak = base::make_weak(controller);
			cSetAutoStart(checked);
			Platform::AutostartToggle(checked, crl::guard(autostart, [=](
					bool enabled) {
				if (checked && !enabled && weak) {
					weak->window().showToast(
						Lang::Hard::AutostartEnableError());
				}
				Ui::PostponeCall(autostart, [=] {
					autostart->setChecked(enabled);
				});
				if (enabled || !minimized->entity()->checked()) {
					Local::writeSettings();
				} else {
					minimized->entity()->setChecked(false);
				}
			}));
		}, autostart->lifetime());

		Platform::AutostartRequestStateFromSystem(crl::guard(
			controller,
			[=](bool enabled) { autostart->setChecked(enabled); }));

		minimized->toggleOn(autostart->checkedValue());
		minimized->entity()->checkedChanges(
		) | rpl::filter([=](bool checked) {
			return (checked != minimizedToggled());
		}) | rpl::on_next([=](bool checked) {
			if (controller->session().domain().local().hasLocalPasscode()) {
				minimized->entity()->setChecked(false);
				controller->show(Ui::MakeInformBox(
					tr::lng_error_start_minimized_passcoded()));
			} else {
				cSetStartMinimized(checked);
				Local::writeSettings();
			}
		}, minimized->lifetime());

		controller->session().domain().local().localPasscodeChanged(
		) | rpl::on_next([=] {
			minimized->entity()->setChecked(minimizedToggled());
		}, minimized->lifetime());
	}

	if (Platform::IsWindows() && !Platform::IsWindowsStoreBuild()) {
		const auto sendto = addCheckbox(
			tr::lng_settings_add_sendto(),
			cSendToMenu());

		sendto->checkedChanges(
		) | rpl::filter([](bool checked) {
			return (checked != cSendToMenu());
		}) | rpl::on_next([](bool checked) {
			cSetSendToMenu(checked);
			psSendToMenu(checked);
			Local::writeSettings();
		}, sendto->lifetime());
	}
}

void SetupAnimations(
		not_null<Window::Controller*> window,
		not_null<Ui::VerticalLayout*> container) {
	container->add(object_ptr<Button>(
		container,
		tr::lng_settings_power_menu(),
		st::settingsButtonNoIcon
	))->setClickedCallback([=] {
		window->show(Box(PowerSavingBox, PowerSaving::Flags()));
	});
}

void ArchiveSettingsBox(
		not_null<Ui::GenericBox*> box,
		not_null<Window::SessionController*> controller) {
	box->setTitle(tr::lng_settings_archive_title());
	box->setWidth(st::boxWideWidth);

	box->addButton(tr::lng_about_done(), [=] { box->closeBox(); });

	PreloadArchiveSettings(&controller->session());

	struct State {
		Ui::SlideWrap<Ui::VerticalLayout> *foldersWrap = nullptr;
		Ui::SettingsButton *folders = nullptr;
	};
	const auto state = box->lifetime().make_state<State>();
	const auto privacy = &controller->session().api().globalPrivacy();

	const auto container = box->verticalLayout();
	AddSkip(container);
	AddSubsectionTitle(container, tr::lng_settings_unmuted_chats());

	using Unarchive = Api::UnarchiveOnNewMessage;
	container->add(object_ptr<Button>(
		container,
		tr::lng_settings_always_in_archive(),
		st::settingsButtonNoIcon
	))->toggleOn(privacy->unarchiveOnNewMessage(
	) | rpl::map(
		rpl::mappers::_1 == Unarchive::None
	))->toggledChanges(
	) | rpl::filter([=](bool toggled) {
		const auto current = privacy->unarchiveOnNewMessageCurrent();
		state->foldersWrap->toggle(!toggled, anim::type::normal);
		return toggled != (current == Unarchive::None);
	}) | rpl::on_next([=](bool toggled) {
		privacy->updateUnarchiveOnNewMessage(toggled
			? Unarchive::None
			: state->folders->toggled()
			? Unarchive::NotInFoldersUnmuted
			: Unarchive::AnyUnmuted);
	}, container->lifetime());

	AddSkip(container);
	AddDividerText(container, tr::lng_settings_unmuted_chats_about());

	state->foldersWrap = container->add(
		object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
			container,
			object_ptr<Ui::VerticalLayout>(container)));
	const auto inner = state->foldersWrap->entity();
	AddSkip(inner);
	AddSubsectionTitle(inner, tr::lng_settings_chats_from_folders());

	state->folders = inner->add(object_ptr<Button>(
		inner,
		tr::lng_settings_always_in_archive(),
		st::settingsButtonNoIcon
	))->toggleOn(privacy->unarchiveOnNewMessage(
	) | rpl::map(
		rpl::mappers::_1 != Unarchive::AnyUnmuted
	));
	state->folders->toggledChanges(
	) | rpl::filter([=](bool toggled) {
		const auto current = privacy->unarchiveOnNewMessageCurrent();
		return toggled != (current != Unarchive::AnyUnmuted);
	}) | rpl::on_next([=](bool toggled) {
		const auto current = privacy->unarchiveOnNewMessageCurrent();
		privacy->updateUnarchiveOnNewMessage(!toggled
			? Unarchive::AnyUnmuted
			: (current == Unarchive::AnyUnmuted)
			? Unarchive::NotInFoldersUnmuted
			: current);
	}, inner->lifetime());

	AddSkip(inner);
	AddDividerText(inner, tr::lng_settings_chats_from_folders_about());

	state->foldersWrap->toggle(
		privacy->unarchiveOnNewMessageCurrent() != Unarchive::None,
		anim::type::instant);

	SetupArchiveAndMute(controller, box->verticalLayout());
}

void PreloadArchiveSettings(not_null<::Main::Session*> session) {
	session->api().globalPrivacy().reload();
}

Type AdvancedId() {
	return Advanced::Id();
}

} // namespace Settings
