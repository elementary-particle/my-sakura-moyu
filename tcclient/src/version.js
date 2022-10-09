export function diffUnit(unit0, unit1) {
    return ((unit0 || unit1) && !(unit0 && unit1)) ||
        unit0.target   !== unit1.target   ||
        unit0.comment  !== unit1.comment  ||
        unit0.state    !== unit1.state    ||
        unit0.location !== unit1.location ||
        unit0.source   !== unit1.source;
}

export function modifyUnit(unitPair, field, value) {
    if (unitPair.prev === unitPair.next) {
        unitPair.next = Object.assign({}, unitPair.prev);
    }
    unitPair.next[field] = value;
    if (!diffUnit(unitPair.prev, unitPair.next)) {
        unitPair.next = unitPair.prev;
    }
}