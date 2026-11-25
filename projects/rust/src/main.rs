use std::io::{self, Write};
use std::process::ExitCode;

fn main() -> ExitCode {
    let mut args = Vec::new();
    for arg in std::env::args_os().skip(1) {
        match arg.into_string() {
            Ok(text) => args.push(text),
            Err(_) => {
                eprintln!("ciutil: argument is not valid UTF-8");
                return ExitCode::from(2);
            }
        }
    }

    let mut stdout = io::stdout().lock();
    let mut stderr = io::stderr().lock();
    let code = ciutil::cli::run(&args, &mut stdout, &mut stderr);
    let _ = stdout.flush();
    ExitCode::from(u8::try_from(code).unwrap_or(1))
}
