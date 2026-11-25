package dev.citest.ciutil;

import java.io.BufferedOutputStream;
import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;

/** process entry point; all behaviour lives in {@link Cli}. */
public final class Main {

    private Main() {}

    public static void main(String[] args) {
        PrintStream out =
                new PrintStream(
                        new BufferedOutputStream(new FileOutputStream(FileDescriptor.out)),
                        false,
                        StandardCharsets.UTF_8);
        PrintStream err =
                new PrintStream(new FileOutputStream(FileDescriptor.err), false, StandardCharsets.UTF_8);
        int code = Cli.run(args, out, err);
        out.flush();
        err.flush();
        System.exit(code);
    }
}
