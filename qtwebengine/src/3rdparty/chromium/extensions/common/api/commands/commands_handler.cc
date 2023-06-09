// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/api/commands/commands_handler.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "extensions/common/command.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/permissions_parser.h"

namespace extensions {

namespace keys = manifest_keys;

namespace {
// The maximum number of commands (including page action/browser actions) with a
// keybinding an extension can have.
const int kMaxCommandsWithKeybindingPerExtension = 4;
}  // namespace

CommandsInfo::CommandsInfo() = default;
CommandsInfo::~CommandsInfo() = default;

// static
const Command* CommandsInfo::GetBrowserActionCommand(
    const Extension* extension) {
  auto* info =
      static_cast<CommandsInfo*>(extension->GetManifestData(keys::kCommands));
  return info ? info->browser_action_command.get() : nullptr;
}

// static
const Command* CommandsInfo::GetPageActionCommand(const Extension* extension) {
  auto* info =
      static_cast<CommandsInfo*>(extension->GetManifestData(keys::kCommands));
  return info ? info->page_action_command.get() : nullptr;
}

// static
const Command* CommandsInfo::GetActionCommand(const Extension* extension) {
  auto* info =
      static_cast<CommandsInfo*>(extension->GetManifestData(keys::kCommands));
  return info ? info->action_command.get() : nullptr;
}

// static
const CommandMap* CommandsInfo::GetNamedCommands(const Extension* extension) {
  auto* info =
      static_cast<CommandsInfo*>(extension->GetManifestData(keys::kCommands));
  return info ? &info->named_commands : nullptr;
}

CommandsHandler::CommandsHandler() = default;
CommandsHandler::~CommandsHandler() = default;

bool CommandsHandler::Parse(Extension* extension, std::u16string* error) {
  if (!extension->manifest()->FindKey(keys::kCommands)) {
    std::unique_ptr<CommandsInfo> commands_info(new CommandsInfo);
    MaybeSetBrowserActionDefault(extension, commands_info.get());
    extension->SetManifestData(keys::kCommands, std::move(commands_info));
    return true;
  }

  const base::DictionaryValue* dict = nullptr;
  if (!extension->manifest()->GetDictionary(keys::kCommands, &dict)) {
    *error = manifest_errors::kInvalidCommandsKey;
    return false;
  }

  std::unique_ptr<CommandsInfo> commands_info(new CommandsInfo);

  int command_index = 0;
  int keybindings_found = 0;
  for (const auto item : dict->GetDict()) {
    ++command_index;

    const base::DictionaryValue* command = nullptr;
    if (!item.second.GetAsDictionary(&command)) {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          manifest_errors::kInvalidKeyBindingDictionary,
          base::NumberToString(command_index));
      return false;
    }

    std::unique_ptr<extensions::Command> binding(new Command());
    if (!binding->Parse(command, item.first, command_index, error))
      return false;  // |error| already set.

    if (binding->accelerator().key_code() != ui::VKEY_UNKNOWN) {
      // Only media keys are allowed to work without modifiers, and because
      // media keys aren't registered exclusively they should not count towards
      // the max of four shortcuts per extension.
      if (!Command::IsMediaKey(binding->accelerator()))
        ++keybindings_found;

      if (keybindings_found > kMaxCommandsWithKeybindingPerExtension &&
          !PermissionsParser::HasAPIPermission(
              extension, mojom::APIPermissionID::kCommandsAccessibility)) {
        *error = ErrorUtils::FormatErrorMessageUTF16(
            manifest_errors::kInvalidKeyBindingTooMany,
            base::NumberToString(kMaxCommandsWithKeybindingPerExtension));
        return false;
      }
    }

    std::string command_name = binding->command_name();
    if (command_name == manifest_values::kBrowserActionCommandEvent) {
      commands_info->browser_action_command = std::move(binding);
    } else if (command_name == manifest_values::kPageActionCommandEvent) {
      commands_info->page_action_command = std::move(binding);
    } else if (command_name == manifest_values::kActionCommandEvent) {
      commands_info->action_command = std::move(binding);
    } else {
      if (command_name[0] != '_')  // All commands w/underscore are reserved.
        commands_info->named_commands[command_name] = *binding;
    }
  }

  MaybeSetBrowserActionDefault(extension, commands_info.get());

  extension->SetManifestData(keys::kCommands, std::move(commands_info));
  return true;
}

bool CommandsHandler::AlwaysParseForType(Manifest::Type type) const {
  return type == Manifest::TYPE_EXTENSION ||
         type == Manifest::TYPE_LEGACY_PACKAGED_APP ||
         type == Manifest::TYPE_PLATFORM_APP;
}

void CommandsHandler::MaybeSetBrowserActionDefault(const Extension* extension,
                                                   CommandsInfo* info) {
  if (extension->manifest()->FindKey(keys::kAction) &&
      !info->action_command.get()) {
    info->action_command =
        std::make_unique<Command>(manifest_values::kActionCommandEvent,
                                  std::u16string(), std::string(), false);
  } else if (extension->manifest()->FindKey(keys::kBrowserAction) &&
             !info->browser_action_command.get()) {
    info->browser_action_command =
        std::make_unique<Command>(manifest_values::kBrowserActionCommandEvent,
                                  std::u16string(), std::string(), false);
  }
}

base::span<const char* const> CommandsHandler::Keys() const {
  static constexpr const char* kKeys[] = {keys::kCommands};
  return kKeys;
}

}  // namespace extensions
