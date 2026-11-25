// swift-tools-version: 6.0
import PackageDescription

// the executable target is not named "ciutil" because that would collide with the
// "CIUtil" library module on case-insensitive file systems; the product name sets the binary name.
let package = Package(
    name: "ciutil",
    products: [
        .library(name: "CIUtil", targets: ["CIUtil"]),
        .executable(name: "ciutil", targets: ["CIUtilCLI"]),
    ],
    targets: [
        .target(name: "CIUtil"),
        .executableTarget(name: "CIUtilCLI", dependencies: ["CIUtil"]),
        .testTarget(name: "CIUtilTests", dependencies: ["CIUtil"]),
    ]
)
