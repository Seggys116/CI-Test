import org.gradle.api.tasks.testing.logging.TestExceptionFormat
import org.gradle.api.tasks.testing.logging.TestLogEvent

plugins {
    application
}

group = "dev.citest"
version = "1.0.0"

val appMainClass = "dev.citest.ciutil.Main"

repositories {
    mavenCentral()
}

dependencies {
    testImplementation(platform(libs.junit.bom))
    testImplementation(libs.junit.jupiter)
    testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

application {
    mainClass.set(appMainClass)
    applicationName = "ciutil"
}

tasks.withType<JavaCompile>().configureEach {
    options.release.set(17)
    options.encoding = "UTF-8"
    options.compilerArgs.addAll(listOf("-Xlint:all", "-Werror"))
}

tasks.jar {
    archiveFileName.set("ciutil.jar")
    manifest {
        attributes(
            "Main-Class" to appMainClass,
            "Implementation-Title" to "ciutil",
            "Implementation-Version" to project.version.toString(),
        )
    }
}

tasks.test {
    useJUnitPlatform()
    defaultCharacterEncoding = "UTF-8"
    testLogging {
        events(TestLogEvent.PASSED, TestLogEvent.SKIPPED, TestLogEvent.FAILED)
        exceptionFormat = TestExceptionFormat.FULL
        showExceptions = true
        showCauses = true
        showStackTraces = true
    }
}
