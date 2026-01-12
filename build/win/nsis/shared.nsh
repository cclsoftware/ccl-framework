;-----------------------------------------------------------------------
; Shared definitions for all installer scripts
;-----------------------------------------------------------------------

!ifndef CCL_BASEDIR
!define CCL_BASEDIR "${BASEDIR}\framework"
!endif

!ifndef NSIS_INCLUDES_DIR
!define NSIS_INCLUDES_DIR "${CCL_BASEDIR}\build\win\nsis"
!endif

!ifndef CCL_APPLICATIONS_DIR
!define CCL_APPLICATIONS_DIR "${BASEDIR}\applications"
!endif

!ifndef CCL_SHARED_APPLICATIONS_DIR
!define CCL_SHARED_APPLICATIONS_DIR "${CCL_APPLICATIONS_DIR}\shared"
!endif

!ifndef CCL_IDENTITIES_DIR
!define CCL_IDENTITIES_DIR "${CCL_BASEDIR}\build\identities"
!endif

!include "${NSIS_INCLUDES_DIR}\ccl.nsh"

!ifdef VENDOR_INCLUDE_DIR
!include "${VENDOR_INCLUDE_DIR}\vendor.nsh"
!endif

!define UNINSTALLER_EXE	"Uninstaller.exe" ; signed wrapper executable for uninstall

;-----------------------------------------------------------------------
; Build Locations
;-----------------------------------------------------------------------

!ifndef BUILDDIR
!ifdef X64
  !define BUILDDIR "${BASEDIR}\build\cmake\win\x64\release"
!else
!ifdef ARM64
  !define BUILDDIR "${BASEDIR}\build\cmake\win\arm64\release"
!else
!ifdef ARM64EC
  !define BUILDDIR "${BASEDIR}\build\cmake\win\arm64ec\release"
!else
!ifdef ARM64X
  !define BUILDDIR "${BASEDIR}\build\cmake\win\arm64\release"
  !define BUILDDIR_MULTIARCH "${BASEDIR}\build\cmake\win\arm64x\release"
!else
  !define BUILDDIR "${BASEDIR}\build\cmake\win\x86\release"
!endif
!endif
!endif
!endif
!endif

!ifndef BUILDDIR_MULTIARCH
  !define BUILDDIR_MULTIARCH "${BUILDDIR}"
!endif

;-----------------------------------------------------------------------
; System architecture definitions
;-----------------------------------------------------------------------

!ifdef X64
  !define X64_COMPATIBLE 1
  !define WIN64
!else
!ifdef ARM64
  !define WIN64
!else
!ifdef ARM64EC
  !define X64_COMPATIBLE 1
  !define WIN64
!else
!ifdef ARM64X
  !define X64_COMPATIBLE 1
  !define WIN64
!else
  !define WIN32
!endif
!endif
!endif
!endif

;-----------------------------------------------------------------------
; Common UI Definitions
;-----------------------------------------------------------------------

!define MUI_HEADERIMAGE_BITMAP_STRETCH			AspectFitHeight
!define MUI_HEADERIMAGE_UNBITMAP_STRETCH		AspectFitHeight
!define MUI_WELCOMEFINISHPAGE_BITMAP_STRETCH	AspectFitHeight
!define MUI_UNWELCOMEFINISHPAGE_BITMAP_STRETCH	AspectFitHeight

;-----------------------------------------------------------------------
; Common installer settings
;-----------------------------------------------------------------------

SetCompressor /SOLID zlib

;-----------------------------------------------------------------------
; Apply window size adjustments to compensate for font metrics
;-----------------------------------------------------------------------

!define MUI_CUSTOMIZE_ADDITIONAL_WIDTH 0u
!define MUI_CUSTOMIZE_ADDITIONAL_HEIGHT -26u

!include "${NSIS_INCLUDES_DIR}\muicustomize.nsh"
!include "${NSIS_INCLUDES_DIR}\functions.nsh"
