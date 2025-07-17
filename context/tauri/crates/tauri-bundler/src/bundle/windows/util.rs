// Copyright 2019-2024 Tauri Programme within The Commons Conservancy
// SPDX-License-Identifier: Apache-2.0
// SPDX-License-Identifier: MIT

use std::{
  fs::create_dir_all,
  path::{Path, PathBuf},
};
use ureq::ResponseExt;

use crate::utils::http_utils::download;

pub const WEBVIEW2_BOOTSTRAPPER_URL: &str = "https://go.microsoft.com/fwlink/p/?LinkId=2124703";
pub const WEBVIEW2_OFFLINE_INSTALLER_X86_URL: &str =
  "https://go.microsoft.com/fwlink/?linkid=2099617";
pub const WEBVIEW2_OFFLINE_INSTALLER_X64_URL: &str =
  "https://go.microsoft.com/fwlink/?linkid=2124701";
pub const WEBVIEW2_URL_PREFIX: &str =
  "https://msedge.sf.dl.delivery.mp.microsoft.com/filestreamingservice/files/";
pub const NSIS_OUTPUT_FOLDER_NAME: &str = "nsis";
pub const NSIS_UPDATER_OUTPUT_FOLDER_NAME: &str = "nsis-updater";
pub const WIX_OUTPUT_FOLDER_NAME: &str = "msi";
pub const WIX_UPDATER_OUTPUT_FOLDER_NAME: &str = "msi-updater";

pub fn webview2_guid_path(url: &str) -> crate::Result<(String, String)> {
  let agent: ureq::Agent = ureq::Agent::config_builder()
    .proxy(ureq::Proxy::try_from_env())
    .build()
    .into();
  let response = agent.head(url).call().map_err(Box::new)?;
  let final_url = response.get_uri().to_string();
  let remaining_url = final_url.strip_prefix(WEBVIEW2_URL_PREFIX).ok_or_else(|| {
    anyhow::anyhow!(
      "WebView2 URL prefix mismatch. Expected `{}`, found `{}`.",
      WEBVIEW2_URL_PREFIX,
      final_url
    )
  })?;
  let (guid, filename) = remaining_url.split_once('/').ok_or_else(|| {
    anyhow::anyhow!(
      "WebView2 URL format mismatch. Expected `<GUID>/<FILENAME>`, found `{}`.",
      remaining_url
    )
  })?;
  Ok((guid.into(), filename.into()))
}

pub fn download_webview2_bootstrapper(base_path: &Path) -> crate::Result<PathBuf> {
  let file_path = base_path.join("MicrosoftEdgeWebview2Setup.exe");
  if !file_path.exists() {
    std::fs::write(&file_path, download(WEBVIEW2_BOOTSTRAPPER_URL)?)?;
  }
  Ok(file_path)
}

pub fn download_webview2_offline_installer(base_path: &Path, arch: &str) -> crate::Result<PathBuf> {
  let url = if arch == "x64" {
    WEBVIEW2_OFFLINE_INSTALLER_X64_URL
  } else {
    WEBVIEW2_OFFLINE_INSTALLER_X86_URL
  };
  let (guid, filename) = webview2_guid_path(url)?;
  let dir_path = base_path.join(guid);
  let file_path = dir_path.join(filename);
  if !file_path.exists() {
    create_dir_all(dir_path)?;
    std::fs::write(&file_path, download(url)?)?;
  }
  Ok(file_path)
}

#[cfg(target_os = "windows")]
pub fn os_bitness<'a>() -> Option<&'a str> {
  use windows_sys::Win32::System::SystemInformation::{
    GetNativeSystemInfo, PROCESSOR_ARCHITECTURE_AMD64, PROCESSOR_ARCHITECTURE_INTEL, SYSTEM_INFO,
  };

  let mut system_info: SYSTEM_INFO = unsafe { std::mem::zeroed() };
  unsafe { GetNativeSystemInfo(&mut system_info) };
  match unsafe { system_info.Anonymous.Anonymous.wProcessorArchitecture } {
    PROCESSOR_ARCHITECTURE_INTEL => Some("x86"),
    PROCESSOR_ARCHITECTURE_AMD64 => Some("x64"),
    _ => None,
  }
}

pub fn patch_binary(binary_path: &PathBuf, package_type: &crate::PackageType) -> crate::Result<()> {
  let file_data = std::fs::read(binary_path)?;
  let mut file_data = file_data; // make mutable

  let pe = match goblin::Object::parse(&file_data)? {
    goblin::Object::PE(pe) => pe,
    _ => {
      return Err(crate::Error::BinaryParseError(
        std::io::Error::new(std::io::ErrorKind::InvalidInput, "binary is not a PE file").into(),
      ));
    }
  };

  let tauri_bundle_section = pe
    .sections
    .iter()
    .find(|s| s.name().unwrap_or_default() == ".taubndl")
    .ok_or(crate::Error::MissingBundleTypeVar)?;

  let data_offset = tauri_bundle_section.pointer_to_raw_data as usize;

  if data_offset + 8 > file_data.len() {
    return Err(crate::Error::BinaryOffsetOutOfRange);
  }

  let ptr_bytes = &file_data[data_offset..data_offset + 8];
  let ptr_value = u64::from_le_bytes(ptr_bytes.try_into().map_err(|_| {
    crate::Error::BinaryParseError(
      std::io::Error::new(std::io::ErrorKind::InvalidData, "invalid pointer bytes").into(),
    )
  })?);

  let rdata_section = pe
    .sections
    .iter()
    .find(|s| s.name().unwrap_or_default() == ".rdata")
    .ok_or_else(|| {
      crate::Error::BinaryParseError(
        std::io::Error::new(std::io::ErrorKind::InvalidInput, ".rdata section not found").into(),
      )
    })?;

  let rva = ptr_value.checked_sub(pe.image_base as u64).ok_or_else(|| {
    crate::Error::BinaryParseError(
      std::io::Error::new(std::io::ErrorKind::InvalidData, "invalid RVA offset").into(),
    )
  })?;

  let file_offset = rdata_section.pointer_to_raw_data as usize
    + (rva as usize).saturating_sub(rdata_section.virtual_address as usize);

  if file_offset + 3 > file_data.len() {
    return Err(crate::Error::BinaryOffsetOutOfRange);
  }

  // Overwrite the string at that offset
  let string_bytes = &mut file_data[file_offset..file_offset + 3];
  match package_type {
    crate::PackageType::Nsis => string_bytes.copy_from_slice(b"NSS"),
    crate::PackageType::WindowsMsi => string_bytes.copy_from_slice(b"MSI"),
    _ => {
      return Err(crate::Error::InvalidPackageType(
        package_type.short_name().to_owned(),
        "windows".to_owned(),
      ));
    }
  }

  std::fs::write(binary_path, &file_data)
    .map_err(|e| crate::Error::BinaryWriteError(e.to_string()))?;

  Ok(())
}
