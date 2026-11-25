import CIUtil
import Foundation

var standardOutput: [UInt8] = []
var standardError: [UInt8] = []
let status = run(
    arguments: Array(CommandLine.arguments.dropFirst()),
    stdout: &standardOutput,
    stderr: &standardError)

// FileHandle writes the raw bytes, so no newline translation happens on Windows
_ = try? FileHandle.standardOutput.write(contentsOf: Data(standardOutput))
_ = try? FileHandle.standardError.write(contentsOf: Data(standardError))
exit(status)
