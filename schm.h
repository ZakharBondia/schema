import qbs

CppApplication {
    qbs.targetPlatform: "macos"

    cpp.compilerIncludePaths: [
    "/usr/local/include/",
    ]
    cpp.compilerFrameworkPaths: ""
    cpp.compilerLibraryPaths: ""

    cpp.cxxFlags: ["-fconcepts", "-Wno-pointer-arith",]

    consoleApplication: true
    files: [
        "src/**/*.cpp",
        "src/**/*.h",
    ]
    cpp.includePaths: [
        "./dep",
        "./src",

        "/usr/local/include/",

    ]

    Group {
        name: "dep"
        files:  "dep/**"
    }

    cpp.dsymutilPath: "/usr/local/Cellar/llvm/8.0.1/bin/dsymutil"
    cpp.cxxLanguageVersion: "c++14"
    cpp.minimumMacosVersion: "10.14"

    Group {     // Properties for the produced executable
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: "bin"
    }
}

