#!/bin/sh
set -eu

mode=${1:-}
name=${2:-}
bin=${3:-./fisics}
case_name=${4:-}

root="examples/projects"

usage() {
  echo "usage: make examples-project NAME=<project>" >&2
  echo "known projects:" >&2
  for dir in "$root"/*; do
    if [ -d "$dir" ]; then
      base=${dir##*/}
      echo "  $base" >&2
    fi
  done
}

if [ -z "$mode" ] || [ -z "$name" ]; then
  usage
  exit 2
fi

project="$root/$name"
if [ ! -d "$project" ]; then
  echo "unknown example project: $name" >&2
  usage
  exit 2
fi

overlay=""
main_src="$project/src/main.c"
extra_src=""
binary="$project/build/$name"

case "$name" in
  compound_growth)
    extra_src="$project/src/compound_growth.c"
    ;;
  kinematics_stepper)
    overlay="--overlay=physics-units"
    extra_src="$project/src/kinematics_stepper.c"
    ;;
  memory_pool_lifecycle)
    overlay="--overlay=memory-check"
    extra_src="$project/src/memory_pool_lifecycle.c"
    ;;
  population_competition)
    extra_src="$project/src/population_competition.c"
    ;;
  collision_1d)
    overlay="--overlay=physics-units"
    extra_src="$project/src/collision_1d.c"
    ;;
  *)
    echo "example project has no runner config: $name" >&2
    exit 2
    ;;
esac

run_valid() {
  mkdir -p "$project/build"
  # shellcheck disable=SC2086
  FISICS_MAX_PROCS=0 "$bin" $overlay -I"$project/src" "$main_src" $extra_src -o "$binary"
  "$binary" > "$project/build/stdout.txt"
  if ! cmp -s "$project/tests/expected_stdout.txt" "$project/build/stdout.txt"; then
    echo "stdout mismatch for $name" >&2
    echo "expected:" >&2
    cat "$project/tests/expected_stdout.txt" >&2
    echo "actual:" >&2
    cat "$project/build/stdout.txt" >&2
    exit 1
  fi
  cat "$project/build/stdout.txt"
  echo "example project passed: $name"
}

run_invalid_one() {
  src=$1
  base=${src##*/}
  stem=${base%.c}
  expected="$project/tests/invalid/$stem.expected.txt"
  output="$project/build/${stem}.stderr.txt"
  object="$project/build/${stem}.o"

  if [ ! -f "$expected" ]; then
    echo "missing expected diagnostic file: $expected" >&2
    exit 1
  fi

  set +e
  # shellcheck disable=SC2086
  FISICS_MAX_PROCS=0 "$bin" $overlay --dump-sema -c "$src" -o "$object" > "$output" 2>&1
  status=$?
  set -e

  if [ "$status" -eq 0 ]; then
    echo "invalid fixture compiled unexpectedly: $src" >&2
    cat "$output" >&2
    exit 1
  fi

  while IFS= read -r expected_line; do
    [ -z "$expected_line" ] && continue
    if ! grep -Fq "$expected_line" "$output"; then
      echo "missing expected diagnostic text in $src: $expected_line" >&2
      cat "$output" >&2
      exit 1
    fi
  done < "$expected"

  echo "invalid fixture rejected as expected: $stem"
}

run_invalid() {
  mkdir -p "$project/build"
  invalid_dir="$project/tests/invalid"
  if [ ! -d "$invalid_dir" ]; then
    echo "project has no invalid fixtures: $name" >&2
    exit 2
  fi

  found=0
  for src in "$invalid_dir"/*.c; do
    [ -f "$src" ] || continue
    stem=${src##*/}
    stem=${stem%.c}
    if [ -n "$case_name" ] && [ "$case_name" != "$stem" ]; then
      continue
    fi
    found=1
    run_invalid_one "$src"
  done

  if [ "$found" -eq 0 ]; then
    echo "no invalid fixture matched CASE=$case_name for $name" >&2
    exit 2
  fi
}

run_artifacts() {
  mkdir -p "$project/build" "$project/ide_files"
  graph="$project/ide_files/build_graph.json"
  FISICS_MAX_PROCS=0 "$bin" --build-manifest "$project/project.fisics.json" --dry-run --json --emit-build-graph-json "$graph"
  # shellcheck disable=SC2086
  FISICS_MAX_PROCS=0 "$bin" $overlay -I"$project/src" "$main_src" $extra_src -o "$binary"
  "$binary" > "$project/build/stdout.txt"
  echo "wrote $graph"
  echo "wrote $project/build/stdout.txt"
}

validate_memory_report() {
  report=$1
  expected=$2
  python3 - "$report" "$expected" <<'PY'
import json
import os
import sys

report_path, expected_path = sys.argv[1], sys.argv[2]
with open(report_path, "r", encoding="utf-8") as handle:
    report = json.load(handle)
with open(expected_path, "r", encoding="utf-8") as handle:
    expected = json.load(handle)

assert report["profile"] == "memory_check_report_v1", report
assert report["schema_version"] == 1, report
assert report["runtime"] == "fisics_memory_check", report
assert report["trigger"] == expected["trigger"], report
assert report["summary"] == expected["summary"], report["summary"]

expected_leaks = expected.get("leaks", [])
actual_leaks = report.get("leaks", [])
assert len(actual_leaks) == len(expected_leaks), actual_leaks
for actual, expected_leak in zip(actual_leaks, expected_leaks):
    assert actual["size"] == expected_leak["size"], actual
    expected_file = expected_leak.get("allocated_file")
    if expected_file:
        actual_file = os.path.basename(actual["allocated_at"]["file"])
        assert actual_file == expected_file, actual
PY
}

run_memory() {
  if [ "$name" != "memory_pool_lifecycle" ]; then
    echo "project has no memory-check mode: $name" >&2
    exit 2
  fi

  mkdir -p "$project/build" "$project/ide_files"

  clean_binary="$project/build/${name}_clean"
  clean_report="$project/ide_files/memory_report_clean.json"
  clean_stderr="$project/build/memory_clean.stderr.txt"

  FISICS_MAX_PROCS=0 "$bin" --overlay=memory-check -I"$project/src" "$main_src" $extra_src -o "$clean_binary"
  env FISICS_MEMCHECK_REPORT=manual FISICS_MEMCHECK_REPORT_JSON="$clean_report" "$clean_binary" \
    > "$project/build/stdout.txt" 2> "$clean_stderr"
  if ! cmp -s "$project/tests/expected_stdout.txt" "$project/build/stdout.txt"; then
    echo "stdout mismatch for $name clean memory run" >&2
    cat "$project/build/stdout.txt" >&2
    exit 1
  fi
  validate_memory_report "$clean_report" "$project/tests/memory/expected_clean_report.json"

  leaky_src="$project/tests/memory/leaky_main.c"
  leaky_binary="$project/build/${name}_leaky"
  leaky_report="$project/ide_files/memory_report_leaky.json"
  leaky_stderr="$project/build/memory_leaky.stderr.txt"

  FISICS_MAX_PROCS=0 "$bin" --overlay=memory-check -I"$project/src" "$leaky_src" $extra_src -o "$leaky_binary"
  env FISICS_MEMCHECK_REPORT=manual FISICS_MEMCHECK_REPORT_JSON="$leaky_report" "$leaky_binary" \
    > "$project/build/leaky_stdout.txt" 2> "$leaky_stderr"
  validate_memory_report "$leaky_report" "$project/tests/memory/expected_leaky_report.json"

  cat "$project/build/stdout.txt"
  echo "memory reports passed: $name"
  echo "wrote $clean_report"
  echo "wrote $leaky_report"
}

run_optional_invalid() {
  invalid_dir="$project/tests/invalid"
  if [ -d "$invalid_dir" ]; then
    run_invalid
  fi
}

write_video_manifest() {
  mkdir -p "$project/build/demo_artifacts"
  manifest="$project/build/demo_artifacts/video_prep_manifest.md"
  {
    echo "# Video Prep: $name"
    echo
    echo "Project root: \`$project\`"
    echo
    echo "## Core Commands"
    echo
    echo "\`\`\`bash"
    echo "make examples-project NAME=$name"
    echo "make examples-project-artifacts NAME=$name"
    if [ -d "$project/tests/invalid" ]; then
      echo "make examples-project-invalid NAME=$name"
    fi
    if [ "$name" = "memory_pool_lifecycle" ]; then
      echo "make examples-project-memory NAME=$name"
    fi
    echo "\`\`\`"
    echo
    echo "## Recording Files"
    echo
    echo "- Story script: \`$project/video_script.md\`"
    echo "- Agent task: \`$project/agent_task.md\`"
    echo "- Expected stdout: \`$project/tests/expected_stdout.txt\`"
    echo "- Captured stdout: \`$project/build/stdout.txt\`"
    echo "- Build graph: \`$project/ide_files/build_graph.json\`"
    if [ -d "$project/tests/invalid" ]; then
      echo "- Invalid fixture output: \`$project/build/*.stderr.txt\`"
    fi
    if [ "$name" = "memory_pool_lifecycle" ]; then
      echo "- Clean memory report: \`$project/ide_files/memory_report_clean.json\`"
      echo "- Leaky memory report: \`$project/ide_files/memory_report_leaky.json\`"
    fi
    echo
    echo "## Acceptance"
    echo
    echo "- Valid run output matches expected stdout."
    echo "- Build graph artifact exists and is nonempty."
    if [ -d "$project/tests/invalid" ]; then
      echo "- Invalid fixture is rejected with the expected diagnostic text."
    fi
    if [ "$name" = "memory_pool_lifecycle" ]; then
      echo "- Clean memory report has no active allocations."
      echo "- Leaky memory report has exactly one active allocation."
    fi
  } > "$manifest"
  echo "wrote $manifest"
}

run_video_prep() {
  run_valid
  run_artifacts
  run_optional_invalid
  if [ "$name" = "memory_pool_lifecycle" ]; then
    run_memory
  fi
  write_video_manifest
}

case "$mode" in
  run)
    run_valid
    ;;
  invalid)
    run_invalid
    ;;
  artifacts)
    run_artifacts
    ;;
  memory)
    run_memory
    ;;
  video-prep)
    run_video_prep
    ;;
  *)
    echo "unknown mode: $mode" >&2
    exit 2
    ;;
esac
