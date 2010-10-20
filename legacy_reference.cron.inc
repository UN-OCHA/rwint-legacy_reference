<?php

// Implementation of hook_cron
function legacy_reference_cron() {
  $field_names = get_legacy_field_info($switch = TRUE);
  $content_types = get_legacy_field_info();
  foreach($content_types as $content_type) {
    foreach($field_names as $field_name) {
      legacy_reference_resolve_legacy_ids($content_type, $field_name);
    }
  }
}

function legacy_reference_resolve_legacy_ids($content_type, $field_name) {
  $last_nid = variable_get('legacy_last_nid', 0);

  $nodes = db_select('node')
    ->fields('node', array('nid'))
    ->condition('type', $content_type)
    ->condition('nid', $last_nid, '>')
    ->range(0, 100)
    ->execute();

  $field_settings = field_info_instance('node', $field_name, $content_type);
  $reference_field = $field_settings['settings']['reference_field'];

  foreach ($nodes as $node) {
    $current_node = node_load($node->nid);
    $legacy_ids = $current_node->{$field_name}['und'][0]['value'];
    $legacy_ids = explode('\n', $legacy_ids);
    $tids = array();
    foreach ($legacy_ids as $lid) {
      $tids = look_up_tids($lid);
    }

    if (isset($tids)) {
      $current_node->{$reference_field}['und'] = $tids;
      node_save($current_node);
    }
    variable_set('legacy_last_nid', $current_node->nid);
  }
}

function get_legacy_field_info($switch = FALSE) {
  $bundles = field_info_bundles('node');

  foreach($bundles as $key => $value) {
    foreach (field_info_instances('node', $key) as $fields) {
      $field = field_info_field($fields['field_name']);
      if ($field['type'] == 'legacy_reference') {
        $field_names[] = $field['field_name'];
        $content_types[] = $field['bundles']['node'];
      }
    }
  }
  if ($switch) {
    return array_unique($field_names);
  }
  else {
    return array_unique($content_types[0]);
  }
}

function look_up_tids($legacy_id) {
  if (isset($legacy_id)) {
    $results = db_select('feeds_item')
    ->fields('feeds_item', array('entity_id'))
    ->condition('guid', $legacy_id, '=')
    ->execute()
    ->fetchAll();

    foreach ($results as $result) {
      $tids[] = array('tid' => $result->entity_id);
    }
    return $tids;
  }
}