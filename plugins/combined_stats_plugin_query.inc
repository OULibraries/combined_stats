<?php
use GuzzleHttp\Message\Request;
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
    if (!isset($row)) {
      //@todo add better error handling here
      //setting server variable

      //setting a start date if start date was deleted
      $time = date('Y-m-d H:i:s', strtotime('-1 hour'));
      $newDate = new DateTime($time, new DateTimeZone('UTC'));
      $newDate->setTimezone(new DateTimeZone('America/Chicago'));

      //number of days, months, etc
      $incNum = $this->options['incnum'];

      //option for days, months, etc
      $spanOption = $this->options['spanOption'];
      $format = $this->options['format'];

      //get the start time for the suma total
      $startDate = $this->getStartTime($incNum, $spanOption);

      // if the $tally variable is set...skip this and use that number in the
      // total at the bottom of the execute function
      if (!isset($tally)) {
        // check to see if the cache is set...if it is skip all this
        if ($cache = cache_get('combined_stats_libanswers_data')) {
          $tally = $cache->data;
        }
        // if not...do all the things
        else {

          //get the username and password for libanswers
          $libanswers_stats_un = variable_get('libanswers_stats_un', '');
          $libanswers_stats_pw = variable_get('libanswers_stats_pw', '');
          if ($libanswers_stats_un == '' || $libanswers_stats_pw == '') {
            return;
          }

          //get the start date and time (if hours are selected) for libanswers
          $libanswersStartDate = $this->getLibanswersStartTime($incNum, $spanOption);
          $libanswersStartHours = ($spanOption == 'hours') ? $this->getHours($incNum, $spanOption) : '';

          //manually setting an array of the different libanswers options
          //these correspond with
          //chat:1753; email:2474; sms:2476; inperson:2628, phone:2629
          $arrayedInteractions = $this->options['interactions'];
          $anid = explode(',', $arrayedInteractions);
          $tally = 0;

          $client = new GuzzleHttp\Client();
          $jar = new GuzzleHttp\Cookie\CookieJar();

          //setting the cookies (I think)
//          try {
//            error_log('hello1');
//            $first = $client->get(
//              'https://ou.libapps.com/libapps/login_process.php', [
//              'cookies' => $jar]
//            );

          $response = $client->get('https://ou.libapps.com/libapps/login_process.php', ['cookies' => $jar]);
          $jar_array = $jar->toArray();
          $new_jar = $jar_array[1]["Value"];

          $response_login = $client->post('https://ou.libapps.com/libapps/login_process.php',
                            [
                              'cookies' => $jar,
                              'allow_redirects' => [
                                'max' => 10,        // allow at most 10 redirects.
                                'strict' => TRUE,      // use "strict" RFC compliant redirects.
                                'track_redirects' => TRUE,
                              ],
                              'body' => [
                                'action' => '1',
                                'site_id' => '0',
                                'app_id' => '0',
                                'target' => '',
                                'public' => '0',
                                'mode' => '0',
                                's-libapps-email' => $libanswers_stats_un,
                                's-libapps-password' => $libanswers_stats_pw,
                                'idtn' => $new_jar,
                              ]
            ]);





            $data = $response_login->getStatusCode();
            if ($data != "200") {
              exit("bad response code, don't log in");
            }


            // passing login info to login page
            // This login just gets us the right cookies to do the next one.
            $response_redirect = $client->get('https://ou.libapps.com/libapps/login.php?site_id=6823',
                                 ['cookies' => $jar]
            );



            //looping through each of the interaction options and getting the total number of interactions
            foreach ($anid as $tempanid) {


            $tempfile = tempnam(sys_get_temp_dir(), 'tempfile');

            //this is going to be hit once for each interaction type
            //then return the data for the csv to be tallied a few lines down
            $response_data = $client->get('http://libanswers.ou.edu/process_tally.php?m=exportxls2&iid=1355&anid=' . $tempanid .
              '&list=stats&start=' . $libanswersStartDate . '&hrstart=' . $libanswersStartHours, [
              'cookies' => $jar,
              'save_to' => $tempfile,
            ]);

            $temp1 = fopen($tempfile, 'r');
            //setting the file position to 0
            fseek($temp1, 0);

            $columnHeads = TRUE;

            //go through the rows and keep a running tally
            while (($csvrow = fgetcsv($temp1)) !== FALSE) {
              //we don't want the first row...which is the column titles
              if ($columnHeads) {
                $columnHeads = FALSE;
                continue;
              }
              $tally++;
            }
          }
            cache_set('combined_stats_libanswers_data', $tally, 'cache', time() + 36000);
        }
      }

      // if $accumulativeCount is set we can skip all this
      if (!isset($accumulativeCount)) {
        // check to see if the cache is set...if it is...skip all the work
        if ($cache = cache_get('combined_stats_suma_stats_data')) {
          $accumulativeCount = $cache->data;
        }
        // if no cache is set...do all the things
        else {
          $accumulativeCount = 0;
          $mySuma = variable_get('combined_suma_stats_server', "");
          if ($mySuma == '') {
            error_log('no longer using the configuration');
          }
          //turn the comma separated list of collections into an array
          $preArrayLocation = $this->options['collection'];
          $arrayedLocation = explode(',', $preArrayLocation);

          //pass the parameters to the api..this may need revised for more options or empty parameters
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
          // set the cache
          cache_set('combined_stats_suma_stats_data', $accumulativeCount, 'cache', time() + 36000);
        }
      }
      $row = new stdClass();
      $row->suma_count = number_format($accumulativeCount);
      $row->libanswers_count = number_format($tally);
      $row->combined_count = number_format($accumulativeCount + $tally);
    }
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

  function getLibanswersStartTime($inc = 0, $span = 0) {
    //setting a start date if start date was deleted
    if ($span == 'hours'){
      $time = date('Y-m-d H:i:s');
    }
    else {
      $time = date('Y-m-d H:i:s', strtotime('-' . $inc . ' ' . $span . ' '));
    }
    $newDate = new DateTime($time, new DateTimeZone('UTC'));
    $newDate->setTimezone(new DateTimeZone('America/Chicago'));
    $tempStartDate = $newDate->format('Y-m-d');
    return $tempStartDate;
  }

  /**
   * @param int $inc
   * @param int $span
   * @return string
   *
   * when hours are chosen libanswers needs an actual date
   * that gives problems when it's on the other side of midnight
   * this function should take care of that
   */
  function getHours($inc = 0, $span = 0) {
    //setting a start hour if hour is the option passed in
    //set the entered time and midnight to the same format
    $time = date('Y-m-d H:i:s', strtotime('-' . $inc . ' ' . $span . ' '));
    $midnight =date('Y-m-d H:i:s', strtotime('today midnight'));

    //set the entered date to central timezone and keep midnight UTC
    $newDate = new DateTime($time, new DateTimeZone('UTC'));
    $newMidnight = new DateTime($midnight, new DateTimeZone('UTC'));
    $newDate->setTimezone(new DateTimeZone('America/Chicago'));

    //change both variables back to the same format for comparison
    $strTime = strtotime($newDate->format('Y-m-d H:i:s'));
    $strMidnight = strtotime($newMidnight->format('Y-m-d H:i:s'));

    //send back a starting hour of zero if the entered time is
    //previous to midnight
    if ($strTime<$strMidnight) {
      $tempStartHour = $newMidnight->format('H');
    }
    else {
      $tempStartHour = $newDate->format('H');
    }
    return $tempStartHour;
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
    $options['interactions'] = array(
      'default' => '1753',
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
      '#description' => t('The ID of the collection that should return stats
      For OU Libraries the options are as follows: Architecture-Services:13, Architecture-Count:14, Bizzell-Desk:<b>1</b>, BLT:<b>24</b>, Bookmark:<b>8</b>, Branches-Service:<b>2</b>,
        Business-Service:<b>22</b>, EDGE-Count:<b>20</b>, EIC Count:<b>6</b>, Engineering-Count:<b>10</b>, Engineering-Service:<b>9</b>, FineArts-Count:<b>16</b>, FineArts-Service:<b>15</b>,
        GovDocs-Service:<b>19</b>, HCLC-Count:<b>5</b>, HCLC-Services:<b>3</b>, HOSService:<b>4</b>, KS-Services:<b>23</b>, WHC-Count:<b>21</b>, Youngblood-Count:<b>7</b>, Youngblood-Service:<b>12</b>'),
      '#default_value' => $this->options['collection'],
      '#maxlength' => 30,
      '#size' => 30,
    );
    $form['interactions'] = array(
      '#type' => 'textfield',
      '#title' => t('Types of interactions'),
      '#description' => t('The anids of the interactions that should return stats.
        For OU Libraries the options are as follows: Chat:<b>1753</b>, Email:<b>2474</b>, SMS:<b>2476</b>, InPerson:<b>2628</b>, Phone:<b>2629</b>.
        Make it a comma separated list with no spaces.'),
      '#default_value' => $this->options['interactions'],
      '#maxlength' => 30,
      '#size' => 30,
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
