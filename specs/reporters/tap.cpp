#include <specs/specs.h>

namespace bd = bandit::detail;

go_bandit([]() {
  describe("reporter::tap", [&]() {
    std::stringstream stm;
    std::unique_ptr<reporter::tap> reporter;

    auto output = [&]() { return stm.str(); };

    before_each([&]() {
      stm.str(std::string());
      reporter.reset(new reporter::tap(stm));
    });

    describe("an empty test run", [&]() {
      before_each([&]() {
        reporter->test_run_starting();
        reporter->test_run_complete();
      });

      it("adds a header to the output", [&]() {
        AssertThat(output(), StartsWith("TAP version 13\n"));
      });

      it("outputs an empty test plan", [&]() {
        AssertThat(output(), Contains("1..0\n"));
      });

      it("ends successfully", [&]() {
        AssertThat(reporter->did_we_pass(), IsTrue());
      });
    });

    describe("a test run with one, successful, test", [&]() {
      before_each([&]() {
        reporter->test_run_starting();
        reporter->context_starting("my context");
        reporter->it_starting("my test");
        reporter->it_succeeded("my test");
        reporter->context_ended("my context");
        reporter->test_run_complete();
      });

      it("outputs a non-empty test plan", [&]() {
        AssertThat(output(), Contains("1..1\n"));
      });

      it("outputs the successful test", [&]() {
        AssertThat(output(), Contains("ok 1 - my test\n"));
      });

      it("ends successfully", [&]() {
        AssertThat(reporter->did_we_pass(), IsTrue());
      });
    });

    describe("a test run with one, failing test", [&]() {
      before_each([&]() {
        reporter->test_run_starting();
        reporter->context_starting("my context");
        reporter->it_starting("my test");

        bd::assertion_exception exception("assertion failed!", "some_file", 123);
        reporter->it_failed("my test", exception);

        reporter->context_ended("my context");
        reporter->test_run_complete();
      });

      it("outputs the failing test", [&]() {
        AssertThat(output(), Contains("not ok 1 - my test\n"));
      });

      it("outputs YAML metadata for the assertion", [&]() {
        AssertThat(output(), Contains(
          "      ---\n"
          "      at:\n"
          "        file: 'some_file'\n"
          "        line: 123\n"
          "      message: \"assertion failed!\"\n"
          "      ...\n"
        ));
      });

      it("does not end successfully", [&]() {
        AssertThat(reporter->did_we_pass(), IsFalse());
      });
    });

    describe("a test run with one test with an unknown error", [&]() {
      before_each([&]() {
        reporter->test_run_starting();
        reporter->context_starting("my context");
        reporter->it_starting("my test");

        reporter->it_unknown_error("my test");

        reporter->context_ended("my context");
        reporter->test_run_complete();
      });

      it("outputs the erroneous test", [&]() {
        AssertThat(output(), Contains("not ok 1 - my test\n"));
      });

      it("outputs YAML metadata for the error", [&]() {
        AssertThat(output(), Contains(
          "      ---\n"
          "      at: ~\n"
          "      message: unknown error!\n"
          "      ...\n"
        ));
      });

      it("does not end successfully", [&]() {
        AssertThat(reporter->did_we_pass(), IsFalse());
      });
    });

    describe("a context with a skipped test", [&]() {
      before_each([&]() {
        reporter->test_run_starting();
        reporter->context_starting("my context");

        reporter->it_starting("my test");
        reporter->it_succeeded("my test");
        reporter->it_skip("my skipped test");

        reporter->context_ended("my context");
        reporter->test_run_complete();
      });

      it("outputs the skipped test", [&]() {
        AssertThat(output(), Contains("ok 2 - my skipped test # SKIP\n"));
      });

      it("ends successfully", [&]() {
        AssertThat(reporter->did_we_pass(), IsTrue());
      });
    });

    describe("a successful test run with nested contexts", [&]() {
      before_each([&]() {
        reporter->test_run_starting();
        reporter->context_starting("context");
        reporter->it_starting("passes");
        reporter->it_succeeded("passes");
        reporter->context_starting("nested context");
        reporter->it_starting("passes");
        reporter->it_succeeded("passes");
        reporter->context_ended("nested context");
        reporter->context_ended("context");
        reporter->test_run_complete();
      });

      it("outputs context diagnostics", [&]() {
        AssertThat(output(), Contains("# Subtest: context\n"));
        AssertThat(output(), Contains("    # Subtest: nested context\n"));
      });

      it("shows the passing tests as passed", [&]() {
        AssertThat(output(), Contains("    ok 1 - passes\n"));
        AssertThat(output(), Contains("        ok 1 - passes\n"));
      });

      it("shows the passing contexts as failed", [&]() {
        AssertThat(output(), Contains("    ok 2 - nested context\n"));
        AssertThat(output(), Contains("ok 1 - context\n"));
      });

      it("ends successfully", [&]() {
        AssertThat(reporter->did_we_pass(), IsTrue());
      });
    });

    describe("a failing test run with nested contexts", [&]() {
      before_each([&]() {
        reporter->test_run_starting();
        reporter->context_starting("context");
        reporter->it_starting("passes");
        reporter->it_succeeded("passes");
        reporter->context_starting("nested context");
        reporter->it_starting("fails");
        bd::assertion_exception exception("assertion failed!", "some_file", 123);
        reporter->it_failed("fails", exception);
        reporter->context_ended("nested context");
        reporter->context_ended("context");
        reporter->test_run_complete();
      });

      it("outputs context diagnostics", [&]() {
        AssertThat(output(), Contains("# Subtest: context\n"));
        AssertThat(output(), Contains("    # Subtest: nested context\n"));
      });

      it("shows the passing test as passed", [&]() {
        AssertThat(output(), Contains("    ok 1 - passes\n"));
      });

      it("shows the failing test as failed", [&]() {
        AssertThat(output(), Contains("        not ok 1 - fails\n"));
      });

      it("outputs YAML metadata for the error", [&]() {
        AssertThat(output(), Contains(
          "          ---\n"
          "          at:\n"
          "            file: 'some_file'\n"
          "            line: 123\n"
          "          message: \"assertion failed!\"\n"
          "          ...\n"
        ));
      });

      it("shows the failing contexts as failed", [&]() {
        AssertThat(output(), Contains("    not ok 2 - nested context\n"));
        AssertThat(output(), Contains("not ok 1 - context\n"));
      });

      it("does not end successfully", [&]() {
        AssertThat(reporter->did_we_pass(), IsFalse());
      });
    });

    describe("a context with test run errors", [&]() {
      before_each([&]() {
        reporter->test_run_starting();
        reporter->context_starting("context");
        bd::test_run_error error("error!");
        reporter->test_run_error("context", error);
        reporter->context_ended("context");
        reporter->test_run_complete();
      });

      it("shows the failing context as failed", [&]() {
        AssertThat(output(), Contains("not ok 1 - context\n"));
      });

      it("outputs YAML metadata for the error", [&]() {
        AssertThat(output(), Contains(
          "  ---\n"
          "  at: ~\n"
          "  message: \"error!\"\n"
          "  ...\n"
        ));
      });

      it("does not end successfully", [&]() {
        AssertThat(reporter->did_we_pass(), IsFalse());
      });
    });
  });
});
