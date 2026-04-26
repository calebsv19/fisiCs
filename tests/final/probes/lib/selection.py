import os


def parse_probe_filters():
    raw = os.environ.get("PROBE_FILTER", "").strip()
    if not raw:
        return []
    return [part.strip() for part in raw.split(",") if part.strip()]


def probe_selected(probe_id, filters):
    if not filters:
        return True
    for token in filters:
        if token.endswith("*"):
            if probe_id.startswith(token[:-1]):
                return True
            continue
        if probe_id == token or probe_id.startswith(token):
            return True
    return False
