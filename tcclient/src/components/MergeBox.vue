<template>
  <tr>
    <th style="width:  5%;">来源</th>
    <th style="width:  5%;">编号</th>
    <th style="width: 30%;">原文</th>
    <th style="width: 30%;">译文</th>
    <th style="width: 15%;">评论</th>
    <th style="width:  5%;">状态</th>
    <th style="width: 10%;">操作</th>
  </tr>
  <tr>
    <td>远程</td>
    <td rowspan="2">{{ unitPair.prev.id }}</td>
    <td>{{ unitPair.prev.source }}</td>
    <td>{{ unitPair.prev.target }}</td>
    <td>{{ unitPair.prev.comment }}</td>
    <td>{{ unitPair.next.state }}</td>
    <td><input type="button" value="复制" v-on:click="copyPrevUnit(unitPair)"></td>
  </tr>
  <tr>
    <td>本地</td>
    <td>{{ unitPair.next.source }}</td>
    <td contenteditable="true" v-text="unitPair.next.target"
        v-on:input="onModify(unitPair, 'target', $event)"></td>
    <td contenteditable="true" v-text="unitPair.next.comment"
        v-on:input="onModify(unitPair, 'comment', $event)"></td>
    <td>{{ unitPair.next.state }}</td>
    <td></td>
  </tr>
</template>

<script>
import {modifyUnit} from "@/version";

export default {
  name: "MergeBox",
  props: {
    unitPair: Object
  },
  methods: {
    copyPrevUnit(unitPair) {
      unitPair.next = unitPair.prev;
    },
    onModify(unitPair, field, $event) {
      modifyUnit(unitPair, field, $event.target.innerText);
    }
  }
}
</script>

<style scoped>

th, td {
  border: 1px inset black;
  font-size: 1em;
  margin: 0;
  padding: 1px 2px;
  text-align: left;
  vertical-align: baseline;
  word-wrap: break-word;
}

</style>