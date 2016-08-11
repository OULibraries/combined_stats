<?php

/**
 * @file
 *   Views query plugin for combined stats.
 */
class combined_stats_plugin_query extends views_plugin_query {

  function query($get_count = FALSE) {
  }

  /**
   * @param view $view
   * gets number of counts for given parameters from suma
   */
  function execute(&$view) {
    //@todo add better error handling here
    //setting server variable
    $mySuma = variable_get('combined_suma_stats_server', "");
    if ($mySuma == '') {
      return;
    }
    //setting a start date if start date was deleted
    $time = date('Y-m-d H:i:s', strtotime('-1 hour'));
    $newDate = new DateTime($time, new DateTimeZone('UTC'));
    $newDate->setTimezone(new DateTimeZone('America/Chicago'));

    //number of days, months, etc
    $incNum = $this->options['incnum'];

    //option for days, months, etc
    $spanOption = $this->options['spanOption'];
    $format = $this->options['format'];
    $startDate = $this->getStartTime($incNum, $spanOption);

    //turn the comma separated list of collections into an array
    $preArrayLocation = $this->options['collection'];
    $arrayedLocation = explode(',', $preArrayLocation);

    //pass the parameters to the api..this may need revised for more options or empty parameters
    $accumulativeCount = 0;
    foreach ($arrayedLocation as $collection) {
      $offset = 0;
      $hasMore = 'true';
      $arrayCount = 0;
      while ($hasMore == 'true') {
        $suma_url = 'https://stats.lib.ou.edu/sumaserver//query/counts?id=' . $collection .
          '&format=%27' . $format . '%27&sdate=' . $startDate . '&offset=' . $offset;
        $response = drupal_http_request($suma_url);
        $sumadata = json_decode($response->data);
        $arrayCount += count($sumadata->initiative->counts);
        $hasMore = ($sumadata->status->{'has more'});
        $offset = ($hasMore == 'true') ? $sumadata->status->offset : 0;
      }
      $accumulativeCount += $arrayCount;
    }
    $row = new stdClass();
    $row->suma_count = $accumulativeCount;
    $view->result[] = $row;
   }

  /**
   * @param int $inc
   * @param int $span
   * @return string
   *
   * set the default date to the proper format
   */
  function getStartTime($inc = 0, $span = 0) {
    //setting a start date if start date was deleted
    $time = date('Y-m-d H:i:s', strtotime('-' . $inc . ' ' . $span . ' '));
    $newDate = new DateTime($time, new DateTimeZone('UTC'));
    $newDate->setTimezone(new DateTimeZone('America/Chicago'));
    $defaultDate = $newDate->format('YmdHi');
    return $defaultDate;
  }

  /**
   * @return array
   *
   * option defaults for query settings
   */
  function option_definition() {
    $options = parent::option_definition();

    $options['collection'] = array(
      'default' => '1',
    );
    $options['format'] = array(
      'default' => 'cal',
    );
    $options['spanOption'] = array(
      'default' => 'years',
    );
    $options['incnum'] = array(
      'default' => '1',
    );

    return $options;
  }

  /**
   * @param $form
   * @param $form_state
   *
   * Creating the fields for the query settings view
   */
  function options_form(&$form, &$form_state) {
    $form = parent::options_form($form, $form_state);

    $form['collection'] = array(
      '#type' => 'textfield',
      '#title' => t('ID of Collection'),
      '#description' => t('The ID of the collection that should return stats'),
      '#default_value' => $this->options['collection'],
      '#maxlength' => 10,
      '#size' => 10,
    );
    $form['format'] = array(
      '#type' => 'textfield',
      '#title' => t('Format'),
      '#description' => t('Choose a combination of C A L '),
      '#default_value' => $this->options['format'],
      '#maxlength' => 10,
      '#size' => 10,
    );
    $form['incnum'] = array(
      '#type' => 'textfield',
      '#title' => t('Incremental Number'),
      '#description' => t('Enter the incremental number to coincide with the span field below'),
      '#default_value' => $this->options['incnum'],
      '#maxlength' => 10,
      '#size' => 10,
    );
    $form['spanOption'] = array(
      '#type' => 'select',
      '#title' => t('Span of time'),
      '#description' => t('Select the option to go with the number entered in the number field above.'),
      '#options' => array(
        'hours' => t('hours ago'),
        'days' => t('days ago'),
        'months' => t('months ago'),
        'years' => t('years ago'),
      ),
      '#default_value' => (isset($this->options['spanOption'])) ? $this->options['spanOption'] : 'days',
    );
  }
}