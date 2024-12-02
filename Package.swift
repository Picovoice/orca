// swift-tools-version:5.3
import PackageDescription
let package = Package(
    name: "Orca-iOS",
    platforms: [
        .iOS(.v13)
    ],
    products: [
        .library(
            name: "Orca",
            targets: ["Orca"]
        )
    ],
    targets: [
        .binaryTarget(
            name: "PvOrca",
            path: "lib/ios/PvOrca.xcframework"
        ),
        .target(
            name: "Orca",
            dependencies: ["PvOrca"],
            path: ".",
            exclude: [
                "binding/ios/OrcaAppTest",
                "demo"
            ],
            sources: [
                "binding/ios/Orca.swift",
                "binding/ios/OrcaErrors.swift"
            ]
        )
    ]
)