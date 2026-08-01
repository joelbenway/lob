window.BENCHMARK_DATA = {
  "lastUpdate": 1785608412730,
  "repoUrl": "https://github.com/joelbenway/lob",
  "entries": {
    "lob instruction counts": [
      {
        "commit": {
          "author": {
            "email": "157863269+joelbenway@users.noreply.github.com",
            "name": "Joel Benway",
            "username": "joelbenway"
          },
          "committer": {
            "email": "157863269+joelbenway@users.noreply.github.com",
            "name": "Joel Benway",
            "username": "joelbenway"
          },
          "distinct": true,
          "id": "9f7dabb3d2b3ed265017401246e66481c72c335f",
          "message": "test: trim redundant asserts to satisfy complexity lint",
          "timestamp": "2026-08-01T18:09:55Z",
          "tree_id": "41f11667b7e7dca8a30bd351f4cf9a16acc5c638",
          "url": "https://github.com/joelbenway/lob/commit/9f7dabb3d2b3ed265017401246e66481c72c335f"
        },
        "date": 1785608412042,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "lob_bench/build_basic",
            "value": 840,
            "unit": "Ir/op"
          },
          {
            "name": "lob_bench/build_full",
            "value": 1994,
            "unit": "Ir/op"
          },
          {
            "name": "lob_bench/build_custom_table",
            "value": 7652.001,
            "unit": "Ir/op"
          },
          {
            "name": "lob_bench/build_boatright",
            "value": 585563.006,
            "unit": "Ir/op"
          },
          {
            "name": "lob_bench/build_zero_search",
            "value": 79386.005,
            "unit": "Ir/op"
          },
          {
            "name": "lob_bench/solve_basic",
            "value": 261181.995,
            "unit": "Ir/op"
          },
          {
            "name": "lob_bench/solve_boatright",
            "value": 261339.01,
            "unit": "Ir/op"
          }
        ]
      }
    ]
  }
}