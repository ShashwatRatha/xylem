use std::collections::HashMap;
use tree_sitter::{Query, QueryCursor, Tree, StreamingIterator};

mod enclosing_fn_extractor;

pub fn get_name_map(src: &String, tree: &Tree, cursor: &mut QueryCursor, query: &Query) -> HashMap<String, Vec<String>> {
    let mut map: HashMap<String, Vec<String>> = HashMap::new();

    let mut matches = cursor.matches(&query, tree.root_node(), src.as_bytes());

    while let Some(m) = matches.next() {
        for capture in m.captures {
            let capture_name = &query.capture_names()[capture.index as usize];
            if *capture_name == "callee"
                && let Ok(callee) = capture.node.utf8_text(src.as_bytes())
            {
                if let Some(enclosing_fn) =
                    enclosing_fn_extractor::find_enclosing_fn(capture.node)
                        && let Some(caller_name) = enclosing_fn_extractor::find_caller_name(
                            enclosing_fn,
                            src.as_bytes(),
                        )
                {
                    map.entry(caller_name.to_string())
                        .or_default()
                        .push(callee.to_string());
                }
            }
        }
    }

    return map;
}
