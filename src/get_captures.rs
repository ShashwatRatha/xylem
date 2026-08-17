// use std::collections::HashMap;
use tree_sitter::{Query, QueryCursor, Range, StreamingIterator, Tree};

pub fn print_function_map(src: &str, tree: &Tree, cursor: &mut QueryCursor, query: &Query) {
    let mut matches = cursor.matches(query, tree.root_node(), src.as_bytes());

    while let Some(m) = matches.next() {
        let mut name = None;
        let mut args = None;
        let mut body = None;
        let mut def_range: Option<Range> = None;

        for capture in m.captures {
            let capture_name = query.capture_names()[capture.index as usize];
            let text = capture.node.utf8_text(src.as_bytes()).ok();

            match capture_name {
                "function.name" => name = text,
                "function.args" => args = text,
                "function.body" => body = text,
                "function.def" => def_range = Some(capture.node.range()),
                _ => {}
            }
        }

        // Guarantees all required captures were found in this match
        if let (Some(name), Some(args), Some(body), Some(range)) = (name, args, body, def_range) {
            println!(
                "{name}{args} {body} RNG: [{} - {}]",
                range.start_point, range.end_point
            );
        }
    }
}
