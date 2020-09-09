#ifndef BANDIT_REPORTERS_TAP_H
#define BANDIT_REPORTERS_TAP_H

#include <iostream>
#include <vector>
#include <cmath>
#include <bandit/reporters/interface.h>

namespace bandit {
  namespace reporter {
    struct tap : public interface {
      tap(std::ostream& stm)
          : success_(true), ctxs_(), stm_(stm) {}

      tap()
          : tap(std::cout) {}

      tap& operator=(const tap&) {
        return *this;
      }

      void test_run_starting() override {
        stm_ << "TAP version 13" << "\n";
        ctxs_.push_back(context{});
        stm_.flush();
      }

      void test_run_complete() override {
        success_ = ctxs_.back().is_success;
        stm_ << "1.." << ctxs_.back().total_specs << "\n";
        ctxs_.pop_back();
        stm_.flush();
      }

      void context_starting(const std::string& desc) override {
        stm_ << indent() << "# Subtest: " << desc << "\n";
        ctxs_.push_back(context{});
        stm_.flush();
      }

      void context_ended(const std::string& desc) override {
        const std::string error = std::move(ctxs_.back().error);
        const bool is_success = ctxs_.back().is_success;
        const char* is_ok = is_success ? "ok" : "not ok";
        stm_ << indent() << "1.." << ctxs_.back().total_specs << "\n";
        ctxs_.pop_back();
        ctxs_.back().is_success = ctxs_.back().is_success && is_success;
        stm_ << indent() << is_ok << " " << (++ctxs_.back().total_specs) << " - " << desc << "\n";
        if (!error.empty()) {
          stm_ << indent() << "  ---\n";
          stm_ << indent() << "  at: ~\n";
          stm_ << indent() << "  message: \"" << escape(error) << "\"\n";
          stm_ << indent() << "  ...\n";
        }
        stm_.flush();
      }

      void test_run_error(const std::string&, const detail::test_run_error& err) override {
        ctxs_.back().is_success = false;
        ctxs_.back().error = err.what();
      }

      void it_starting(const std::string&) override {
        // No-op.
      }

      void it_skip(const std::string& desc) override {
        stm_ << indent() << "ok " << (++ctxs_.back().total_specs) << " - " << desc << " # SKIP\n";
        stm_.flush();
      }

      void it_succeeded(const std::string& desc) override {
        stm_ << indent() << "ok " << (++ctxs_.back().total_specs) << " - " << desc << "\n";
        stm_.flush();
      }

      void it_failed(const std::string& desc, const detail::assertion_exception& ex) override {
        ctxs_.back().is_success = false;
        stm_ << indent() << "not ok " << (++ctxs_.back().total_specs) << " - " << desc << "\n";
        stm_ << indent() << "  ---\n";
        stm_ << indent() << "  at:\n";
        stm_ << indent() << "    file: '" << ex.file_name() << "'\n";
        stm_ << indent() << "    line: " << ex.line_number() << "\n";
        stm_ << indent() << "  message: \"" << escape(ex.what()) << "\"\n";
        stm_ << indent() << "  ...\n";
        stm_.flush();
      }

      void it_unknown_error(const std::string& desc) override {
        ctxs_.back().is_success = false;
        stm_ << indent() << "not ok " << (++ctxs_.back().total_specs) << " - " << desc << "\n";
        stm_ << indent() << "  ---\n";
        stm_ << indent() << "  at: ~\n";
        stm_ << indent() << "  message: unknown error!\n";
        stm_ << indent() << "  ...\n";
        stm_.flush();
      }

      bool did_we_pass() const override {
        return ctxs_.empty() ? success_ : ctxs_.back().is_success;
      }

    private:
      std::string escape(const std::string& str) {
        std::stringstream stm;

        for (char c : str) {
          switch (c) {
          case '\n':
            stm << "\\n";
            break;
          case '\r':
            stm << "\\r";
            break;
          case '\t':
            stm << "\\t";
            break;
          case '\'':
            stm << "\\'";
            break;
          case '\\':
            stm << "\\\\";
            break;
          case '\"':
            stm << "\\\"";
            break;
          default:
            stm << c;
          }
        }

        return stm.str();
      }

      std::string indent() {
        int level = std::max(static_cast<std::size_t>(1u), ctxs_.size()) - 1u;
        return std::string(4 * level, ' ');
      }

      struct context {
        int total_specs = 0;
        bool is_success = true;
        std::string error = "";
      };

      bool success_;
      std::vector<context> ctxs_;
      std::ostream& stm_;
    };
  }
}
#endif
