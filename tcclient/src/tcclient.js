import $ from 'jquery';

export class TCClient {
    BATCH_SIZE = 1000;

    constructor(api) {
        this.api = api;
    }

    requestJson(uri, method, data) {
        if (data && method !== "GET") {
            data = JSON.stringify(data);
        }
        return $.ajax({
            type: method,
            url: this.api + uri,
            data: data,
            contentType: "application/json",
            dataType: "json"
        });
    }

    getProjectList() {
        return this.requestJson("", "GET", null).then(
            (result) => (result.code === 0 ? result.object : null)
        );
    }

    getRange(projectName) {
        return this.requestJson("/" + projectName + "/range", "GET").then(
            (result) => (result.code === 0 ? result.object : null)
        );
    }

    getUnitsByRange(projectName, start, end) {
        return this.requestJson("/" + projectName + "/batch/" + start + "_" + end, "GET").then(
            (result) => (result.code === 0 ? result.object : null)
        );
    }

    postSlice(projectName, units, start, end) {
        return this.requestJson("/" + projectName + "/batch",
            "POST", units.slice(start, end)).then(
            (result) => (result.code === 0)
        );
    }

    postUnits(projectName, units) {
        let i = 0, j = this.BATCH_SIZE;
        let requests = [];
        while (j < units.length) {
            requests.push(this.postSlice(projectName, units, i, j));
            i = j;
            j = i + this.BATCH_SIZE;
        }
        requests.push(this.postSlice(projectName, units, i, units.length));

        return Promise.all(requests).then((results) => {
            let allResult = true;
            results.forEach((result) => {
                allResult = allResult && result;
            });
            return allResult;
        });
    }

    getUnit(projectName, id) {
        return this.requestJson("/" + projectName + "/" + id, "GET").then(
            (result) => (result.code === 0 ? result.object : null)
        );
    }

    postUnit(projectName, unit) {
        return this.requestJson("/" + projectName + "/" + unit.id, "POST", unit).then(
            (result) => (result.code === 0)
        );
    }
}
