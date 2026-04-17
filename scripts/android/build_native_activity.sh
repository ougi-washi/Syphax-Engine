#!/bin/bash

set -euo pipefail

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
BUILD_TYPE="Release"
ABI="arm64-v8a"
API="24"
MODE="apk"
APP_ID="com.syphax.engine"
APP_NAME="Syphax Engine"
LIB_NAME="syphax_android_app"
ENTRY_SOURCE="$ROOT_DIR/examples/hello_text.c"
SDK_ROOT="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
NDK_ROOT="${ANDROID_NDK_ROOT:-${ANDROID_NDK_HOME:-}}"
VERSION_CODE="1"
VERSION_NAME="0.1.0"
KEYSTORE=""
KEY_ALIAS=""
STORE_PASS=""
KEY_PASS=""
BUNDLETOOL_BIN="${BUNDLETOOL:-bundletool}"
BUILD_ALL_EXAMPLES="OFF"

show_help() {
	echo "Usage: scripts/android/build_native_activity.sh [options]"
	echo ""
	echo "Options:"
	echo "  --mode=apk|aab|both|native   Output type (default: apk)"
	echo "  --abi=<abi>                  Android ABI (default: arm64-v8a)"
	echo "  --api=<level>                Android API level (default: 24)"
	echo "  --build-type=Debug|Release   Native build type (default: Release)"
	echo "  --app-id=<id>                Android application id"
	echo "  --app-name=<name>            Android application label"
	echo "  --lib-name=<name>            NativeActivity library name"
	echo "  --entry-source=<file>        C example entry source (default: examples/hello_text.c)"
	echo "  --sdk-root=<dir>             Android SDK root"
	echo "  --ndk-root=<dir>             Android NDK root"
	echo "  --version-code=<n>           Android versionCode (default: 1)"
	echo "  --version-name=<name>        Android versionName (default: 0.1.0)"
	echo "  --keystore=<file>            Keystore used for APK signing"
	echo "  --key-alias=<name>           Key alias for signing"
	echo "  --store-pass=<pass>          Keystore password"
	echo "  --key-pass=<pass>            Key password"
	echo "  --bundletool=<path>          bundletool binary or jar path"
	echo "  --all-examples              Also compile every example as Android shared libraries"
	echo "  --help                       Show this help"
	echo ""
	echo "Notes:"
	echo "  - APK mode packages repo resources into APK assets/resources/."
	echo "  - AAB mode also packages resources into the base module today."
	echo "  - OBB is not generated; the Android-first path uses APK assets and AAB."
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--mode=*)
			MODE="${1#*=}"
			shift
			;;
		--abi=*)
			ABI="${1#*=}"
			shift
			;;
		--api=*)
			API="${1#*=}"
			shift
			;;
		--build-type=*)
			BUILD_TYPE="${1#*=}"
			shift
			;;
		--app-id=*)
			APP_ID="${1#*=}"
			shift
			;;
		--app-name=*)
			APP_NAME="${1#*=}"
			shift
			;;
		--lib-name=*)
			LIB_NAME="${1#*=}"
			shift
			;;
		--entry-source=*)
			ENTRY_SOURCE="${1#*=}"
			shift
			;;
		--sdk-root=*)
			SDK_ROOT="${1#*=}"
			shift
			;;
		--ndk-root=*)
			NDK_ROOT="${1#*=}"
			shift
			;;
		--version-code=*)
			VERSION_CODE="${1#*=}"
			shift
			;;
		--version-name=*)
			VERSION_NAME="${1#*=}"
			shift
			;;
		--keystore=*)
			KEYSTORE="${1#*=}"
			shift
			;;
		--key-alias=*)
			KEY_ALIAS="${1#*=}"
			shift
			;;
		--store-pass=*)
			STORE_PASS="${1#*=}"
			shift
			;;
		--key-pass=*)
			KEY_PASS="${1#*=}"
			shift
			;;
		--bundletool=*)
			BUNDLETOOL_BIN="${1#*=}"
			shift
			;;
		--all-examples)
			BUILD_ALL_EXAMPLES="ON"
			shift
			;;
		--help|-h)
			show_help
			exit 0
			;;
		*)
			echo "Unknown option: $1" >&2
			show_help
			exit 1
			;;
	esac
done

if [[ "$ENTRY_SOURCE" != /* ]]; then
	ENTRY_SOURCE="$ROOT_DIR/$ENTRY_SOURCE"
fi

case "$MODE" in
	apk|aab|both|native)
		;;
	*)
		echo "Unsupported mode: $MODE" >&2
		exit 1
		;;
esac

find_sdk_root() {
	local candidates=()
	if [[ -n "${SDK_ROOT:-}" ]]; then
		candidates+=("$SDK_ROOT")
	fi
	candidates+=(
		"$HOME/Android/Sdk"
		"$HOME/Library/Android/sdk"
		"/opt/android-sdk"
		"/usr/lib/android-sdk"
	)
	for candidate in "${candidates[@]}"; do
		if [[ -d "$candidate/platforms" ]]; then
			echo "$candidate"
			return 0
		fi
	done
	return 1
}

find_ndk_root() {
	if [[ -n "${NDK_ROOT:-}" && -f "$NDK_ROOT/build/cmake/android.toolchain.cmake" ]]; then
		echo "$NDK_ROOT"
		return 0
	fi
	if [[ -n "${SDK_ROOT:-}" ]]; then
		if [[ -f "$SDK_ROOT/ndk-bundle/build/cmake/android.toolchain.cmake" ]]; then
			echo "$SDK_ROOT/ndk-bundle"
			return 0
		fi
		if [[ -d "$SDK_ROOT/ndk" ]]; then
			local latest_ndk=""
			latest_ndk="$(find "$SDK_ROOT/ndk" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1 || true)"
			if [[ -n "$latest_ndk" && -f "$latest_ndk/build/cmake/android.toolchain.cmake" ]]; then
				echo "$latest_ndk"
				return 0
			fi
		fi
	fi
	return 1
}

require_command() {
	if ! command -v "$1" >/dev/null 2>&1; then
		echo "Missing required command: $1" >&2
		exit 1
	fi
}

find_build_tools_dir() {
	local root="$1"
	if [[ ! -d "$root/build-tools" ]]; then
		return 1
	fi
	find "$root/build-tools" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1
}

find_bundletool_bin() {
	if command -v "$BUNDLETOOL_BIN" >/dev/null 2>&1; then
		command -v "$BUNDLETOOL_BIN"
		return 0
	fi

	local candidates=(
		"$HOME/.local/share/android-tools/bundletool/bundletool-all-"*.jar
		"$HOME/.local/share/android-tools/bundletool/bundletool.jar"
		"$HOME/.local/bin/bundletool"
	)
	for candidate in "${candidates[@]}"; do
		if [[ -f "$candidate" || -x "$candidate" ]]; then
			echo "$candidate"
			return 0
		fi
	done
	return 1
}

run_bundletool() {
	if [[ "$BUNDLETOOL_BIN" == *.jar ]]; then
		java -jar "$BUNDLETOOL_BIN" "$@"
	else
		"$BUNDLETOOL_BIN" "$@"
	fi
}

ensure_debug_keystore() {
	local debug_keystore="$BUILD_DIR/debug.keystore"
	if [[ -n "$KEYSTORE" ]]; then
		return 0
	fi
	require_command keytool
	mkdir -p "$(dirname "$debug_keystore")"
	if [[ ! -f "$debug_keystore" ]]; then
		keytool -genkeypair \
			-v \
			-keystore "$debug_keystore" \
			-storepass android \
			-keypass android \
			-alias androiddebugkey \
			-dname "CN=Android Debug,O=Android,C=US" \
			-keyalg RSA \
			-keysize 2048 \
			-validity 10000 >/dev/null
	fi
	KEYSTORE="$debug_keystore"
	KEY_ALIAS="${KEY_ALIAS:-androiddebugkey}"
	STORE_PASS="${STORE_PASS:-android}"
	KEY_PASS="${KEY_PASS:-android}"
}

SDK_ROOT="$(find_sdk_root)" || {
	echo "Unable to find Android SDK root. Set --sdk-root or ANDROID_SDK_ROOT." >&2
	exit 1
}
NDK_ROOT="$(find_ndk_root)" || {
	echo "Unable to find Android NDK root. Set --ndk-root or ANDROID_NDK_ROOT." >&2
	exit 1
}

ANDROID_JAR="$SDK_ROOT/platforms/android-$API/android.jar"
if [[ ! -f "$ANDROID_JAR" ]]; then
	echo "Missing Android platform jar: $ANDROID_JAR" >&2
	exit 1
fi

BUILD_TOOLS_DIR="$(find_build_tools_dir "$SDK_ROOT")" || {
	echo "Unable to find Android build-tools under $SDK_ROOT/build-tools" >&2
	exit 1
}

AAPT2="$BUILD_TOOLS_DIR/aapt2"
ZIPALIGN="$BUILD_TOOLS_DIR/zipalign"
APKSIGNER="$BUILD_TOOLS_DIR/apksigner"

for tool_path in "$AAPT2" "$ZIPALIGN" "$APKSIGNER"; do
	if [[ ! -x "$tool_path" ]]; then
		echo "Required Android build tool is missing: $tool_path" >&2
		exit 1
	fi
done

if [[ "$MODE" == "aab" || "$MODE" == "both" ]]; then
	BUNDLETOOL_BIN="$(find_bundletool_bin)" || {
		echo "Unable to find bundletool. Use --bundletool or install it under ~/.local/share/android-tools/bundletool." >&2
		exit 1
	}
fi

BUILD_DIR="$ROOT_DIR/build/android/$ABI"
PACKAGE_DIR="$BUILD_DIR/package"
GENERATED_DIR="$BUILD_DIR/android"
GENERATED_RES_DIR="$GENERATED_DIR/res"
MANIFEST_PATH="$GENERATED_DIR/AndroidManifest.xml"
NATIVE_LIB_PATH="$ROOT_DIR/bin/android/$ABI/lib$LIB_NAME.so"

COMPILED_RES_ZIP="$PACKAGE_DIR/compiled-res.zip"
UNSIGNED_RESOURCE_APK="$PACKAGE_DIR/$LIB_NAME-resources-unsigned.apk"
UNALIGNED_APK="$PACKAGE_DIR/$LIB_NAME-unaligned.apk"
ALIGNED_APK="$PACKAGE_DIR/$LIB_NAME-aligned.apk"
SIGNED_APK="$PACKAGE_DIR/$LIB_NAME-$ABI.apk"
PROTO_APK="$PACKAGE_DIR/$LIB_NAME-proto.apk"
BASE_MODULE_DIR="$PACKAGE_DIR/base-module"
BASE_ZIP="$PACKAGE_DIR/base.zip"
AAB_PATH="$PACKAGE_DIR/$LIB_NAME-$ABI.aab"

mkdir -p "$PACKAGE_DIR"

echo "Configuring Android native build..."
cmake \
	-S "$ROOT_DIR" \
	-B "$BUILD_DIR" \
	-DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
	-DCMAKE_TOOLCHAIN_FILE="$NDK_ROOT/build/cmake/android.toolchain.cmake" \
	-DANDROID_ABI="$ABI" \
	-DANDROID_PLATFORM="android-$API" \
	-DSE_BACKEND_RENDER=gles \
	-DSE_BACKEND_PLATFORM=android \
	-DSE_BUILD_EXAMPLES=OFF \
	-DSE_ANDROID_ENTRY_SOURCE="$ENTRY_SOURCE" \
	-DSE_ANDROID_APP_ID="$APP_ID" \
	-DSE_ANDROID_APP_NAME="$APP_NAME" \
	-DSE_ANDROID_LIB_NAME="$LIB_NAME" \
	-DSE_ANDROID_MIN_SDK="$API" \
	-DSE_ANDROID_TARGET_SDK="$API" \
	-DSE_ANDROID_VERSION_CODE="$VERSION_CODE" \
	-DSE_ANDROID_VERSION_NAME="$VERSION_NAME" \
	-DSE_ANDROID_BUILD_ALL_EXAMPLES="$BUILD_ALL_EXAMPLES"

echo "Building Android native library..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --target "$LIB_NAME"

if [[ "$BUILD_ALL_EXAMPLES" == "ON" ]]; then
	echo "Building Android compile-only example sweep..."
	cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --target se_android_all_examples
fi

if [[ "$MODE" == "native" ]]; then
	echo "Native build complete: $NATIVE_LIB_PATH"
	if [[ "$BUILD_ALL_EXAMPLES" == "ON" ]]; then
		echo "Android example libraries: $ROOT_DIR/bin/android/$ABI/examples"
	fi
	exit 0
fi

if [[ ! -f "$MANIFEST_PATH" ]]; then
	echo "Generated manifest missing: $MANIFEST_PATH" >&2
	exit 1
fi
if [[ ! -f "$NATIVE_LIB_PATH" ]]; then
	echo "Native library missing: $NATIVE_LIB_PATH" >&2
	exit 1
fi

stage_runtime_payload() {
	local target_dir="$1"
	rm -rf "$target_dir"
	mkdir -p "$target_dir/lib/$ABI" "$target_dir/assets"
	cp "$NATIVE_LIB_PATH" "$target_dir/lib/$ABI/"
	cp -R "$ROOT_DIR/resources" "$target_dir/assets/"
}

compile_resources() {
	require_command zip
	rm -f "$COMPILED_RES_ZIP"
	"$AAPT2" compile --dir "$GENERATED_RES_DIR" -o "$COMPILED_RES_ZIP"
}

build_apk() {
	require_command zip
	ensure_debug_keystore
	compile_resources
	"$AAPT2" link \
		--manifest "$MANIFEST_PATH" \
		--auto-add-overlay \
		-I "$ANDROID_JAR" \
		-o "$UNSIGNED_RESOURCE_APK" \
		"$COMPILED_RES_ZIP"

	local runtime_stage="$PACKAGE_DIR/runtime-apk"
	stage_runtime_payload "$runtime_stage"
	cp "$UNSIGNED_RESOURCE_APK" "$UNALIGNED_APK"
	(
		cd "$runtime_stage"
		zip -qr "$UNALIGNED_APK" lib assets
	)

	"$ZIPALIGN" -f 4 "$UNALIGNED_APK" "$ALIGNED_APK"
	"$APKSIGNER" sign \
		--ks "$KEYSTORE" \
		--ks-key-alias "$KEY_ALIAS" \
		--ks-pass "pass:$STORE_PASS" \
		--key-pass "pass:$KEY_PASS" \
		--out "$SIGNED_APK" \
		"$ALIGNED_APK"

	echo "Signed APK: $SIGNED_APK"
}

build_aab() {
	require_command zip
	require_command unzip
	require_command java
	compile_resources
	"$AAPT2" link \
		--proto-format \
		--manifest "$MANIFEST_PATH" \
		--auto-add-overlay \
		-I "$ANDROID_JAR" \
		-o "$PROTO_APK" \
		"$COMPILED_RES_ZIP"

	local proto_dir="$PACKAGE_DIR/proto-apk"
	rm -rf "$proto_dir" "$BASE_MODULE_DIR" "$BASE_ZIP"
	mkdir -p "$proto_dir"
	unzip -q "$PROTO_APK" -d "$proto_dir"

	stage_runtime_payload "$BASE_MODULE_DIR"
	mkdir -p "$BASE_MODULE_DIR/manifest"
	cp "$proto_dir/AndroidManifest.xml" "$BASE_MODULE_DIR/manifest/AndroidManifest.xml"
	if [[ -d "$proto_dir/res" ]]; then
		cp -R "$proto_dir/res" "$BASE_MODULE_DIR/"
	fi
	if [[ ! -f "$proto_dir/resources.pb" ]]; then
		echo "resources.pb missing from proto APK output" >&2
		exit 1
	fi
	cp "$proto_dir/resources.pb" "$BASE_MODULE_DIR/resources.pb"

	(
		cd "$BASE_MODULE_DIR"
		zip -qr "$BASE_ZIP" .
	)

	rm -f "$AAB_PATH"
	run_bundletool build-bundle --modules="$BASE_ZIP" --output="$AAB_PATH"
	echo "App Bundle: $AAB_PATH"
}

if [[ "$MODE" == "apk" || "$MODE" == "both" ]]; then
	build_apk
fi

if [[ "$MODE" == "aab" || "$MODE" == "both" ]]; then
	build_aab
fi
