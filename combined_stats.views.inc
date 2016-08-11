<?php

/**
 * Implements hook_views_plugins().
 */
function combined_stats_views_plugins() {
  $plugin = array();
  $plugin['query']['combined_stats_plugin_query'] = array(
    'title' => t('Combined Suma Stats Query'),
    'help' => t('Combined Suma Stats query object.'),
    'handler' => 'combined_stats_plugin_query',
  );
  return $plugin;
}

/**
 * Implements hook_views_data().
 * 
 * the actual fields from which the user can choose in the view
 */
function combined_stats_views_data() {
  $data = array();

  //Base data
  $data['combined_stats']['table']['group'] = t('Combined Suma Stats');
  $data['combined_stats']['table']['base'] = array(
    'title' => t('Combined Suma Stats'),
    'help' => t('combined Query Suma Stats'),
    'query class' => 'combined_stats_plugin_query'
  );

  //Fields
  $data['combined_stats']['suma_count'] = array(
    'title' => t('Suma Count'),
    'help' => t('The Suma count of this location'),
    'field' => array(
      'handler' => 'combined_stats_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  return $data;
}