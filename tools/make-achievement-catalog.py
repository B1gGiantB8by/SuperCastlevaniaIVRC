"""Regenerate the C catalog from the checked-in offline achievement metadata."""
import json
from pathlib import Path
root = Path(__file__).resolve().parent.parent
rows = json.loads((root / 'assets/castlevania/achievements/catalog.json').read_text())
assert len(rows) == 37 and sum(row['points'] for row in rows) == 593
assert len({row['id'] for row in rows}) == len(rows)
(root / 'src/achievement_catalog.inc').write_text('\n'.join(
    '{%d,%s,%s,%d,%s},' % (row['id'], json.dumps(row['name']),
                         json.dumps(row['description']), row['points'], json.dumps(row['badge']))
    for row in rows) + '\n')
