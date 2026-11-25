using CiUtil;

using Stream stdout = Console.OpenStandardOutput();
using Stream stderr = Console.OpenStandardError();
return Cli.Run(args, stdout, stderr);
