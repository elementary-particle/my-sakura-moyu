<template>
  <div class="app-window" style="width: 100%;" v-if="!hasConflict">
    <table>
      <tr>
        <td>项目</td>
        <td><select class="options" v-model="projectName" v-on:change="onChangeProject">
          <option v-for="projectName in projectList" :key="projectName">
            {{ projectName }}
          </option>
        </select></td>
        <td>编号范围</td>
        <td><input class="options" style="width: 5em" type="number"
                   v-on:change="constraintRange" v-model="unitRangeStart"></td>
        <td>到</td>
        <td><input class="options" style="width: 5em" type="number"
                   v-on:change="constraintRange" v-model="unitRangeEnd"></td>
        <td>显示范围</td>
        <td><input class="options" style="width: 5em" type="number"
                   v-on:change="updateView" v-model="viewRangeStart"></td>
        <td>长度</td>
        <td><input class="options" style="width: 5em" type="number"
                   v-on:change="updateView" v-model="viewRangeLength"></td>
      </tr>
    </table>
    <table>
      <tr>
        <td>工作表</td>
        <td><input class="options" type="file" v-on:change="onChangeBook($event)"></td>
      </tr>
    </table>
    <div style="width: inherit;">
      <input class="control" type="button" value="拉取" v-on:click="onPull">
      <input class="control" type="button" value="推送" v-on:click="onTryPush">
      <input class="control" type="button" value="导入" v-on:click="loadBook">
      <input class="control" type="button" value="导出" v-on:click="saveBook">
    </div>
    <unit-list v-bind:unit-pair-list="viewUnitList"></unit-list>
  </div>
  <div style="width: 100%" v-if="hasConflict">
    <div>远程与本地的修改出现了冲突，请在下列表格的“本地”行中输入合并方案。</div>
    <input type="button" value="合并修改" v-on:click="mergeConflict">
    <table style="width: inherit;">
      <merge-box v-for="unitPair in conflictList" :key="unitPair"
                 v-bind:unit-pair="unitPair"></merge-box>
    </table>
  </div>
</template>

<script>
import * as XLSX from "xlsx";
import {TCClient} from "@/tcclient";
import {diffUnit} from "@/version";
import UnitList from "@/components/UnitList";
import MergeBox from "@/components/MergeBox";

function storageAvailable(type) {
  let storage;
  try {
    storage = window[type];
    const x = '__storage_test__';
    storage.setItem(x, x);
    storage.removeItem(x);
    return true;
  } catch (e) {
    return e instanceof DOMException && (
            // everything except Firefox
            e.code === 22 ||
            // Firefox
            e.code === 1014 ||
            // test name field too, because code might not be present
            // everything except Firefox
            e.name === 'QuotaExceededError' ||
            // Firefox
            e.name === 'NS_ERROR_DOM_QUOTA_REACHED') &&
        // acknowledge QuotaExceededError only if there's something already stored
        (storage && storage.length !== 0);
  }
}

export default {
  name: "App",
  components: {MergeBox, UnitList},
  data: () => ({
    bookFile: null,
    conflictList: [],
    hasConflict: false,
    hasLocalStorage: false,
    projectName: "",
    projectList: [],
    waitPush: false,
    tcClient: null,
    unitIndex: {},
    unitRangeStart: 1,
    unitRangeEnd: 1,
    viewRangeStart: 1,
    viewRangeLength: 50,
    viewUnitList: [],
  }),
  mounted() {
    this.tcClient = new TCClient("/api");
    this.tcClient.getProjectList().then(
        (list) => {
          if (list) {
            this.projectList = list;
          }
        }
    );
    this.hasLocalStorage = storageAvailable("localStorage");
    if (this.hasLocalStorage) {
      if (localStorage.getItem("__used__")) {
        this.projectName = localStorage.getItem("projectName");
        this.unitRangeStart = parseInt(localStorage.getItem("unitRangeStart"));
        this.unitRangeEnd = parseInt(localStorage.getItem("unitRangeEnd"));
        this.viewRangeStart = parseInt(localStorage.getItem("viewRangeStart"));
        this.viewRangeLength = parseInt(localStorage.getItem("viewRangeLength"));
      }
      window.addEventListener("beforeunload", this.onLeave);
    }
    if (this.projectName) {
      this.onChangeProject();
    }
  },
  unmounted() {
    if (this.hasLocalStorage) {
      window.removeEventListener("beforeunload", this.onLeave);
    }
  },
  methods: {
    showError(message) {
      alert(message);
    },
    constraintRange() {
      if (this.unitRangeStart < 1) {
        this.unitRangeStart = 1;
      }
      if (this.unitRangeStart > this.unitRangeEnd) {
        this.unitRangeEnd = this.unitRangeStart;
      }
    },
    onChangeBook($event) {
      this.bookFile = $event.target.files[0];
    },
    onChangeProject() {
      this.conflictList = [];
      this.hasConflict = false;
      this.waitPush = false;
      this.unitIndex = {};
      this.tcClient.getRange(this.projectName).then(
          (range) => {
            if (range) {
              this.unitRangeStart = range.min;
              this.unitRangeEnd = range.max;
              this.updateView();
              this.onPull();
            }
          }
      );
    },
    refreshView() {
      this.viewUnitList = [];
      for (let i = this.viewRangeStart;
           i < this.viewRangeStart + this.viewRangeLength; i++) {
        const unitPair = this.unitIndex[i];
        if (unitPair) {
          this.viewUnitList.push(unitPair);
        }
      }
    },
    onPull() {
      const vue = this;

      if (!this.projectName) {
        this.showError("错误:未选择项目");
        return;
      }

      return this.tcClient.getUnitsByRange(this.projectName, this.unitRangeStart, this.unitRangeEnd).then(
          (list) => {
            if (list) {
              let hasConflict = false;
              vue.conflictList = [];
              list.forEach((unit) => {
                const id = unit.id;
                const unitPair = this.unitIndex[id] || (this.unitIndex[id] = {});
                if (diffUnit(unitPair.prev, unit)) {
                  if (!unitPair.next || unitPair.prev === unitPair.next ||
                      !diffUnit(unitPair.prev, unitPair.next)) {
                    unitPair.next = unit;
                    unitPair.prev = unit;
                  } else {
                    if (diffUnit(unitPair.next, unit)) {
                      hasConflict = true;
                      vue.conflictList.push(unitPair);
                      unitPair.prev = unit;
                    } else {
                      unitPair.prev = unitPair.next;
                    }
                  }
                }
              });
              vue.hasConflict = hasConflict;
              if (!hasConflict && vue.waitPush) {
                vue.onPush();
              }
              vue.refreshView();
            }
          }
      );
    },
    onPush() {
      if (!this.projectName) {
        this.showError("错误:未选择项目");
        return;
      }

      const unitPairs = [];
      for (let i = this.unitRangeStart; i <= this.unitRangeEnd; i++) {
        const unitPair = this.unitIndex[i];
        if (unitPair) {
          console.assert(unitPair.prev || unitPair.next);
          if (!unitPair.next) {
            unitPair.next = unitPair.prev;
          } else if (unitPair.prev !== unitPair.next) {
            unitPairs.push(unitPair);
          }
        }
      }
      return this.tcClient.postUnits(this.projectName, unitPairs.map((pair) => (pair.next))).then(
          (result) => {
            if (result) {
              unitPairs.forEach((pair) => {
                pair.prev = pair.next;
              });
            }
          }
      );
    },
    mergeConflict() {
      this.conflictList = [];
      this.hasConflict = false;
      if (this.waitPush) {
        this.waitPush = false;
        this.onPush();
      }
    },
    onTryPush() {
      this.waitPush = true;
      this.onPull();
    },
    loadBook() {
      if (!this.bookFile) {
        this.showError("错误:未选择任何文件");
        return;
      }
      const reader = new FileReader();
      const vue = this;
      reader.onload = function () {
        const book = XLSX.read(this.result);
        let minId = vue.unitRangeStart, maxId = vue.unitRangeEnd;
        if (book.Sheets.prev && book.Sheets.next) {
          XLSX.utils.sheet_to_json(book.Sheets.prev, {defval: ""}).forEach(
              (unit) => {
                const id = unit.id;
                const unitPair = vue.unitIndex[id] || (vue.unitIndex[id] = {});
                unitPair.prev = unit;
              }
          );
          XLSX.utils.sheet_to_json(book.Sheets.next, {defval: ""}).forEach(
              (unit) => {
                const unitPair = vue.unitIndex[unit.id] || (vue.unitIndex[unit.id] = {});
                if (diffUnit(unitPair.prev, unit)) {
                  unitPair.next = unit;
                } else {
                  unitPair.next = unitPair.prev;
                }
                if (minId > unit.id) {
                  minId = unit.id;
                }
                if (maxId < unit.id) {
                  maxId = unit.id;
                }
              }
          );
          vue.unitRangeStart = minId;
          vue.unitRangeEnd = maxId;
        } else {
          /* fallback */
          const sheet = book.Sheets[book.SheetNames[0]];
          XLSX.utils.sheet_to_json(sheet, {defval: ""}).forEach(
              (unit) => {
                const id = unit.id;
                if (unit.update) {
                  const unitPair = vue.unitIndex[id] || (vue.unitIndex[id] = {
                    prev: Object.assign({}, unit)
                  });
                  unit.update = undefined;
                  unitPair.next = unit;
                }
              }
          );
        }
        vue.refreshView();
      }
      reader.readAsArrayBuffer(this.bookFile);
    },
    saveBook() {
      const prevUnits = [], nextUnits = [];
      for (let i = this.unitRangeStart; i <= this.unitRangeEnd; i++) {
        const unitPair = this.unitIndex[i];
        if (unitPair) {
          prevUnits.push(unitPair.prev);
          nextUnits.push(unitPair.next);
        }
      }
      const book = XLSX.utils.book_new();
      const columnWidths = [6, 10, 50, 50, 30, 6].map((col) => ({"wch": col}));
      const prevSheet = XLSX.utils.json_to_sheet(prevUnits, {
        header: ["id", "location", "source", "target", "comment", "state"]
      });
      prevSheet["!cols"] = columnWidths;
      const nextSheet = XLSX.utils.json_to_sheet(prevUnits, {
        header: ["id", "location", "source", "target", "comment", "state"]
      });
      nextSheet["!cols"] = columnWidths;
      XLSX.utils.book_append_sheet(book, nextSheet, "next");
      XLSX.utils.book_append_sheet(book, prevSheet, "prev");
      XLSX.writeFile(book, this.projectName + "_" +
          this.unitRangeStart + "_" + this.unitRangeEnd + ".xlsx");
    },
    updateView() {
      if (this.viewRangeStart < this.unitRangeStart) {
        this.viewRangeStart = this.unitRangeStart;
      }
      if (this.viewRangeLength < 0) {
        this.viewRangeLength = 0;
      }
      if (this.viewRangeLength > this.unitRangeEnd - this.viewRangeStart + 1) {
        this.viewRangeLength = this.unitRangeEnd - this.viewRangeStart + 1;
      }
      this.refreshView();
    },
    onLeave() {
      localStorage.setItem("__used__", "true");
      localStorage.setItem("projectName", this.projectName);
      localStorage.setItem("unitRangeStart", this.unitRangeStart);
      localStorage.setItem("unitRangeEnd", this.unitRangeEnd);
      localStorage.setItem("viewRangeStart", this.viewRangeStart);
      localStorage.setItem("viewRangeLength", this.viewRangeLength);
    }
  }
};
</script>

<style>

.options {
  border: none;
  box-sizing: border-box;
  font-size: 1em;
  height: 2em;
  padding: 1px 2px;
}

.control {
  border: 1px groove black;
  font-size: large;
  height: 1.6em;
  margin: 5px;
  width: 15%;
}

.app-window {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
}
</style>
