// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// A helper library to keep track of a user's key by SID.
// Used by RLZ libary. Also to be used by SearchWithGoogle library.

#include "rlz/win/lib/user_key.h"

#include "base/process_util.h"
#include "base/utf_string_conversions.h"
#include "base/win/registry.h"
#include "base/win/windows_version.h"
#include "rlz/lib/assert.h"
#include "rlz/win/lib/process_info.h"

namespace rlz_lib {

bool RegKeyReadValue(base::win::RegKey& key, const wchar_t* name,
                     char* value, size_t* value_size) {
  value[0] = 0;

  std::wstring value_string;
  if (key.ReadValue(name, &value_string) != ERROR_SUCCESS) {
    return false;
  }

  if (value_string.length() > *value_size) {
    *value_size = value_string.length();
    return false;
  }

  // Note that RLZ string are always ASCII by design.
  strncpy(value, WideToUTF8(value_string).c_str(), *value_size);
  value[*value_size - 1] = 0;
  return true;
}

bool RegKeyWriteValue(base::win::RegKey& key, const wchar_t* name,
                      const char* value) {
  std::wstring value_string(ASCIIToWide(value));
  return key.WriteValue(name, value_string.c_str()) == ERROR_SUCCESS;
}

bool HasUserKeyAccess(bool write_access) {
  // The caller is trying to access HKEY_CURRENT_USER.  Test to see if we can
  // read from there.  Don't try HKEY_CURRENT_USER because this will cause
  // problems in the unit tests: if we open HKEY_CURRENT_USER directly here,
  // the overriding done for unit tests will no longer work.  So we try subkey
  // "Software" which is known to always exist.
  base::win::RegKey key;
  if (key.Open(HKEY_CURRENT_USER, L"Software", KEY_READ) != ERROR_SUCCESS)
    ASSERT_STRING("Could not open HKEY_CURRENT_USER");

  if (ProcessInfo::IsRunningAsSystem()) {
    ASSERT_STRING("UserKey::HasAccess: No access as SYSTEM without SID set.");
    return false;
  }

  if (write_access) {
    if (base::win::GetVersion() < base::win::VERSION_VISTA) return true;
    base::ProcessHandle process_handle = base::GetCurrentProcessHandle();
    base::IntegrityLevel level = base::INTEGRITY_UNKNOWN;

    if (!base::GetProcessIntegrityLevel(process_handle, &level)) {
      ASSERT_STRING("UserKey::HasAccess: Cannot determine Integrity Level.");
      return false;
    }
    if (level <= base::LOW_INTEGRITY) {
      ASSERT_STRING("UserKey::HasAccess: Cannot write from Low Integrity.");
      return false;
    }
  }
  return true;
}

}  // namespace rlz_lib
