<?php

require('../include/json-header.php');

function fetch_runs_for_config_and_test_group($db, $config, $test_group_id) {
    return $db->query_and_fetch_all('
        SELECT test_runs.*, builds.*, array_agg((commit_repository, commit_revision, commit_time)) AS revisions
            FROM builds
                LEFT OUTER JOIN build_commits ON commit_build = build_id
                LEFT OUTER JOIN commits ON build_commit = commit_id,
                test_runs, build_requests, analysis_test_groups
            WHERE run_build = build_id AND run_config = $1 AND request_build = build_id AND request_group = $2
            GROUP BY build_id, run_id', array($config['config_id'], $test_group_id));
}

function fetch_runs_for_config($db, $config) {
    return $db->query_and_fetch_all('
        SELECT test_runs.*, builds.*, array_agg((commit_repository, commit_revision, commit_time)) AS revisions
            FROM builds
                LEFT OUTER JOIN build_commits ON commit_build = build_id
                LEFT OUTER JOIN commits ON build_commit = commit_id, test_runs
            WHERE run_build = build_id AND run_config = $1 AND NOT EXISTS (SELECT * FROM build_requests WHERE request_build = build_id)
            GROUP BY build_id, run_id', array($config['config_id']));
}

function main($path) {
    if (count($path) != 1)
        exit_with_error('InvalidRequest');

    $parts = explode('-', $path[0]);
    if (count($parts) != 2)
        exit_with_error('InvalidRequest');

    $db = new Database;
    if (!$db->connect())
        exit_with_error('DatabaseConnectionFailure');

    $platform_id = intval($parts[0]);
    $metric_id = intval($parts[1]);
    $config_rows = $db->query_and_fetch_all('SELECT *
        FROM test_configurations WHERE config_metric = $1 AND config_platform = $2', array($metric_id, $platform_id));
    if (!$config_rows)
        exit_with_error('ConfigurationNotFound');

    $test_group_id = array_get($_GET, 'testGroup');
    $should_cache = array_get($_GET, 'cache');
    if ($test_group_id)
        $test_group_id = intval($test_group_id);
    else if ($should_cache) { // Only v1 UI needs caching.
        $maxage = config('jsonCacheMaxAge');
        header('Expires: ' . gmdate('D, d M Y H:i:s', time() + $maxage) . ' GMT');
        header("Cache-Control: maxage=$maxage");
    }

    $generator = new RunsGenerator($config_rows);

    foreach ($config_rows as $config) {
        if ($test_group_id)
            $raw_runs = fetch_runs_for_config_and_test_group($db, $config, $test_group_id);
        else
            $raw_runs = fetch_runs_for_config($db, $config);
        $generator->add_runs($config['config_type'], $raw_runs);
    }

    $content = success_json($generator->results());
    if (!$test_group_id)
        generate_data_file("$platform_id-$metric_id.json", $content);
    echo $content;
}

class RunsGenerator {
    function __construct($config_rows) {
        $this->results = array();
        $last_modified_times = array();
        foreach ($config_rows as $row)
            array_push($last_modified_times, Database::to_js_time($row['config_runs_last_modified']));
        $this->last_modified = max($last_modified_times);
        $this->start_time = microtime(true);
    }

    function results() {
        return array(
            'configurations' => &$this->results,
            'lastModified' => $this->last_modified,
            'elapsedTime' => (microtime(true) - $this->start_time) * 1000);
    }

    function add_runs($name, $raw_runs) {
        $formatted_runs = array();
        if ($raw_runs) {
            foreach ($raw_runs as $run)
                array_push($formatted_runs, self::format_run($run));
        }
        $this->results[$name] = $formatted_runs;
        return $formatted_runs;
    }

    private static function format_run($run) {
        return array(
            'id' => intval($run['run_id']),
            'mean' => floatval($run['run_mean_cache']),
            'iterationCount' => intval($run['run_iteration_count_cache']),
            'sum' => floatval($run['run_sum_cache']),
            'squareSum' => floatval($run['run_square_sum_cache']),
            'markedOutlier' => Database::is_true($run['run_marked_outlier']),
            'revisions' => self::parse_revisions_array($run['revisions']),
            'build' => $run['build_id'],
            'buildTime' => Database::to_js_time($run['build_time']),
            'buildNumber' => intval($run['build_number']),
            'builder' => $run['build_builder']);
    }

    private static function parse_revisions_array($postgres_array) {
        // e.g. {"(WebKit,131456,\"2012-10-16 14:53:00\")","(Chromium,162004,)"}
        $outer_array = json_decode('[' . trim($postgres_array, '{}') . ']');
        $revisions = array();
        foreach ($outer_array as $item) {
            $name_and_revision = explode(',', trim($item, '()'));
            if (!$name_and_revision[0])
                continue;
            $time = Database::to_js_time(trim($name_and_revision[2], '"'));
            $revisions[trim($name_and_revision[0], '"')] = array(trim($name_and_revision[1], '"'), $time);
        }
        return $revisions;
    }
}

main(array_key_exists('PATH_INFO', $_SERVER) ? explode('/', trim($_SERVER['PATH_INFO'], '/')) : array());

?>
