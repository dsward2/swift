// First build without chaining.
// RUN: %empty-directory(%t)
// RUN: %target-build-swift-dylib(%t/%target-library-name(A)) -module-name A -emit-module -emit-module-path %t -swift-version 5 %S/Inputs/dynamic_replacement_chaining_A.swift
// RUN: %target-build-swift-dylib(%t/%target-library-name(B)) -I%t -L%t -lA %target-rpath(%t) -module-name B -emit-module -emit-module-path %t -swift-version 5 %S/Inputs/dynamic_replacement_chaining_B.swift
// RUN: %target-build-swift-dylib(%t/%target-library-name(C)) -I%t -L%t -lA %target-rpath(%t) -module-name C -emit-module -emit-module-path %t -swift-version 5 %S/Inputs/dynamic_replacement_chaining_B.swift
// RUN: %target-build-swift -I%t -L%t -lA -o %t/main %target-rpath(%t) %s -swift-version 5
// RUN: %target-codesign %t/main %t/%target-library-name(A) %t/%target-library-name(B) %t/%target-library-name(C)
// RUN: %target-run %t/main %t/%target-library-name(A) %t/%target-library-name(B) %t/%target-library-name(C)

// Now build with chaining enabled.
// RUN: %empty-directory(%t)
// RUN: %target-build-swift-dylib(%t/%target-library-name(A)) -module-name A -emit-module -emit-module-path %t -swift-version 5 %S/Inputs/dynamic_replacement_chaining_A.swift
// RUN: %target-build-swift-dylib(%t/%target-library-name(B)) -I%t -L%t -lA %target-rpath(%t) -module-name B -emit-module -emit-module-path %t -swift-version 5 %S/Inputs/dynamic_replacement_chaining_B.swift -Xfrontend -enable-dynamic-replacement-chaining
// RUN: %target-build-swift-dylib(%t/%target-library-name(C)) -I%t -L%t -lA %target-rpath(%t) -module-name C -emit-module -emit-module-path %t -swift-version 5 %S/Inputs/dynamic_replacement_chaining_B.swift -Xfrontend -enable-dynamic-replacement-chaining
// RUN: %target-build-swift -I%t -L%t -lA -DCHAINING -o %t/main %target-rpath(%t) %s -swift-version 5
// RUN: %target-codesign %t/main %t/%target-library-name(A) %t/%target-library-name(B) %t/%target-library-name(C)
// RUN: %target-run %t/main %t/%target-library-name(A) %t/%target-library-name(B) %t/%target-library-name(C)

// REQUIRES: executable_test

// This test flips the chaining flag.
// UNSUPPORTED: swift_test_mode_optimize_none_with_implicit_dynamic
// UNSUPPORTED: wasm

import A

import StdlibUnittest

#if os(macOS) || os(iOS) || os(tvOS) || os(watchOS)
  import Darwin
#elseif os(Linux) || os(FreeBSD) || os(OpenBSD) || os(PS4) || os(Android) || os(Cygwin) || os(Haiku)
  import Glibc
#elseif os(Windows)
  import MSVCRT
  import WinSDK
#else
#error("Unsupported platform")
#endif

var DynamicallyReplaceable = TestSuite("DynamicallyReplaceableChaining")

func target_library_name(_ name: String) -> String {
#if os(iOS) || os(macOS) || os(tvOS) || os(watchOS)
  return "lib\(name).dylib"
#elseif os(Windows)
  return "\(name).dll"
#else
  return "lib\(name).so"
#endif
}

DynamicallyReplaceable.test("DynamicallyReplaceable") {
  var executablePath = CommandLine.arguments[0]
  executablePath.removeLast(4)

#if os(Linux)
  _ = dlopen(target_library_name("B"), RTLD_NOW)
  _ = dlopen(target_library_name("C"), RTLD_NOW)
#elseif os(Windows)
  _ = LoadLibraryA(target_library_name("B"))
  _ = LoadLibraryA(target_library_name("C"))
#else
  _ = dlopen(executablePath+target_library_name("B"), RTLD_NOW)
  _ = dlopen(executablePath+target_library_name("C"), RTLD_NOW)
#endif

#if CHAINING
  expectEqual(3, Impl().foo())
#else
  expectEqual(2, Impl().foo())
#endif
}

runAllTests()
