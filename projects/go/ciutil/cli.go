package ciutil

import (
	"errors"
	"fmt"
	"io"
	"strconv"
)

const (
	ExitError = 2
	ExitUsage = 64
)

const usage = `usage: ciutil <command> [args]

commands:
  crc32 STRING        print the CRC-32 of STRING as 8 hex digits
  adler32 STRING      print the Adler-32 of STRING as 8 hex digits
  b64enc STRING       print STRING encoded as base64
  b64dec BASE64       write the decoded bytes to stdout
  roman NUMBER        print NUMBER (1-3999) as a roman numeral
  unroman ROMAN       print a roman numeral as a decimal number
  semver-cmp A B      print -1, 0 or 1 comparing semantic versions A and B
  version             print the program version
`

var errUsage = errors.New("usage")

// Run executes the CLI. args excludes the program name. It returns 0 on
// success, ExitError for invalid input and ExitUsage for misuse.
func Run(args []string, stdout, stderr io.Writer) int {
	err := dispatch(args, stdout)
	switch {
	case err == nil:
		return 0
	case errors.Is(err, errUsage):
		_, _ = io.WriteString(stderr, usage)
		return ExitUsage
	default:
		_, _ = fmt.Fprintf(stderr, "ciutil: %v\n", err)
		return ExitError
	}
}

func dispatch(args []string, out io.Writer) error {
	if len(args) == 0 {
		return errUsage
	}
	command, operands := args[0], args[1:]
	switch {
	case command == "version" && len(operands) == 0:
		_, err := io.WriteString(out, "ciutil go 1.0.0\n")
		return err
	case command == "crc32" && len(operands) == 1:
		_, err := fmt.Fprintf(out, "%08x\n", CRC32Checksum([]byte(operands[0])))
		return err
	case command == "adler32" && len(operands) == 1:
		_, err := fmt.Fprintf(out, "%08x\n", Adler32Checksum([]byte(operands[0])))
		return err
	case command == "b64enc" && len(operands) == 1:
		_, err := io.WriteString(out, Base64Encode([]byte(operands[0]))+"\n")
		return err
	case command == "b64dec" && len(operands) == 1:
		decoded, err := Base64Decode(operands[0])
		if err != nil {
			return err
		}
		_, err = out.Write(decoded)
		return err
	case command == "roman" && len(operands) == 1:
		n, err := strconv.Atoi(operands[0])
		if err != nil {
			return fmt.Errorf("invalid number %q", operands[0])
		}
		numeral, err := ToRoman(n)
		if err != nil {
			return err
		}
		_, err = io.WriteString(out, numeral+"\n")
		return err
	case command == "unroman" && len(operands) == 1:
		n, err := FromRoman(operands[0])
		if err != nil {
			return err
		}
		_, err = fmt.Fprintf(out, "%d\n", n)
		return err
	case command == "semver-cmp" && len(operands) == 2:
		a, err := ParseVersion(operands[0])
		if err != nil {
			return fmt.Errorf("invalid version %q: %w", operands[0], err)
		}
		b, err := ParseVersion(operands[1])
		if err != nil {
			return fmt.Errorf("invalid version %q: %w", operands[1], err)
		}
		_, err = fmt.Fprintf(out, "%d\n", a.Compare(b))
		return err
	default:
		return errUsage
	}
}
