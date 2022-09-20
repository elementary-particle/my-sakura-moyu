<template>
  <div class="mdui-table-fluid">
    <table class="translation mdui-table mdui-table-hoverable">
      <thead>
        <tr>
          <th style="width:  5%;">编号</th>
          <th style="width: 35%;">原文</th>
          <th style="width: 35%;">译文</th>
          <th style="width: 20%;">评论</th>
          <th style="width:  5%;">状态</th>
        </tr>
      </thead>
      <tbody>
        <tr v-for="unitPair in unitPairList" :key="unitPair"
            v-bind:style="{background: unitPair.prev === unitPair.next? '#dfd': '#fdd'}">
          <td>{{ unitPair.next.id }}</td>
          <td>{{ unitPair.next.source }}</td>
          <td contenteditable="true" v-text="unitPair.next.target"
              v-on:input="onModify(unitPair, 'target', $event)"></td>
          <td contenteditable="true" v-text="unitPair.next.comment"
              v-on:input="onModify(unitPair, 'comment', $event)"></td>
          <td contenteditable="true" v-text="unitPair.next.state"
              v-on:input="modifyUnitState(unitPair, $event)" class="mdui-text-center"></td>
        </tr>
      </tbody>
    </table>
  </div>
</template>

<script>
import {modifyUnit} from "@/version";

export default {
  name: "UnitList",
  props: {
    unitPairList: Array
  },
  methods: {
    onModify(unitPair, field, $event) {
      modifyUnit(unitPair, field, $event.target.innerText);
    },
    modifyUnitState(unitPair, $event) {
      const state = parseInt($event.target.innerText);
      if (!isNaN(state)) {
        modifyUnit(unitPair, 'state', state);
      } else {
        $event.target.innerText = unitPair.next.state;
      }
    }
  }
}
</script>

<style scoped>

th, td {
  margin: auto;
  padding: 4px 6px;
}

</style>