<?php

/**
 * Plugin for "apachesolr_geo" query types.
 */
//TODO: rename class to compy with facetapi naming convention
class FacetapiApachesolrGeoQueryType extends FacetapiQueryType implements FacetapiQueryTypeInterface {
  /**
   * Returns the query type associated with the plugin.
   *
   * @return string
   *   The query type.
   */
  static public function getType() {
    return 'apachesolr_geo';
  }

  public function extract($item) {
    list($type, $value) = explode('.', $item['value'], 2);
    switch($type) {
      case 'pt':
        list($latlng, $place) = explode(':', $value, 2);
        list($lat, $lng) = explode(',', $latlng, 2);
        return array('latitude' => $lat, 'longitude' => $lng, 'place' => $place);
      case 'd':
        return array('distance' => $value);
    }
    return array();
  }
  /**
   * Adds the filter to the query object.
   *
   * @param DrupalSolrQueryInterface $query
   *   An object containing the query in the backend's native API.
   */
  public function execute($query) {
    // Retrieve settings of the facet.
    // We should be able to get all constants as facet options.

    $field_name = $this->facet['field'];
    // See:  http://wiki.apache.org/solr/StatsComponent
    //$query->addParam('stats', 'true');
    //$query->addParam('stats.field', $field_name);
    //$query->addParam('stats.facet', $field_name);
    // Range filters don't support OR operator.

    $settings = $this->getSettings()->settings; //$this->adapter->getFacet($this->facet)->getSettings();

    $distance_options =  !empty($settings['apachesolr_geo_facet_options']) ? $settings['apachesolr_geo_facet_options'] : APACHESOLR_GEO_FACET_OPTIONS;

    $facet_distances = explode(',', $distance_options);

    $active_items = $this->adapter->getActiveItems($this->facet);

    $items_by_type = $this->getItemsByType($active_items);

     //replace with isset
    if (empty($items_by_type['d'])) {
      $distance = $settings['distance_bounds_max'];
    }
    else {
      $active_item = end($items_by_type['d']);
      $distance = $active_item['distance'];
      // Add current selected distance to have possibility to unselect it.
      //$facet_distances[] = 1; //todo, need this?
    }
    if(empty($items_by_type['pt'])) {
      //$central_point = APACHESOLR_GEO_SEARCH_CENTER_POINT;
    }
    else {
      $active_item = end($items_by_type['pt']);
      $central_point = $active_item['latitude'] . ',' .  $active_item['longitude'];
    }
    if(isset($central_point)) {

      $query->setAvailableSort('geodist()', array('title' => t('Distance'), 'default' => 'asc'));

      if(empty($_GET['solrsort'])) {
        $query->setSolrSort('geodist()', 'asc');
      }

      // Search center point.
      $query->addParam('pt', $central_point);

      // Set location field name. //TODO, can we include everything in the one field query below?
      $query->addParam('sfield', $this->facet['field']);

      $tag = $field_name . '_geofilt';
      $query->addParam('fq', '{!geofilt sfield=' . $this->facet['field'] . ' tag=' . $tag . '}');

      // Set search radius.
      $query->addParam('d', $distance);

      if(
        !empty($items_by_type['d']) &&
        !empty($settings['apachesolr_geo_facet_max_depth']) &&
        count($items_by_type['d']) >= $settings['apachesolr_geo_facet_max_depth'] // TODO: should in fact never be larger than, provide better validation
      ) {
        array_pop($items_by_type['d']);
        //TODO: fix this duplicate code, move to method?
        if (empty($items_by_type['d'])) {
          $distance = $settings['distance_bounds_max'];
        }
        else {
          $active_item = end($items_by_type['d']);
          $distance = $active_item['distance'];
        }
        // Set facets and exclude current geofilter
        foreach ($facet_distances as $facet_option) {
          $facet_distance = $distance * $facet_option;
          $query->addParam('facet.query', '{!geofilt d=' . $facet_distance . ' key=' . $field_name . '.d.' . $facet_distance . ' ex=' . $tag . '}');
        }
      }
      else {
         // Set facets
        foreach ($facet_distances as $facet_option) {
          $facet_distance = $distance * $facet_option;
          $query->addParam('facet.query', '{!geofilt d=' . $facet_distance . ' key=' . $field_name . '.d.' . $facet_distance . '}');
        }
      }
    }
  }

  protected function getItemsByType($items) {
    $items_by_type = array();
    foreach($items as $key => $item) {
      list($type, $value) = explode('.', $key, 2);
      $items_by_type[$type][$key] = $item;
    }
    return $items_by_type;
  }

  public static function settingsForm(&$form, &$form_state, $settings) {
    parent::settingsForm($form, $form_state);

    //Options
    // use geofilt or bbox (faster but inaccurate)
    //dsm($form);
    //TODO: Prefix with module name
    $form['distance_bounds_max'] = array(
      '#title' => t('The default distance'),
      '#description' => t('The default maximum distance in kilometers.'),
      '#type' => 'textfield',
      '#default_value' => isset($settings['distance_bounds_max']) ? $settings['distance_bounds_max'] : APACHESOLR_GEO_DEFAULT_RADIUS,
    );

    $form['apachesolr_geo_facet_options'] = array(
      '#title' => t('Facet options'),
      //'#description' => t('The default maximum distance in kilometers.'),
      '#type' => 'textfield',
      '#default_value' => isset($settings['apachesolr_geo_facet_options']) ? $settings['apachesolr_geo_facet_options'] : APACHESOLR_GEO_FACET_OPTIONS,
    );

    $form['apachesolr_geo_facet_max_depth'] = array(
      '#title' => t('Max depth'),
      //'#description' => t('The default maximum distance in kilometers.'),
      '#type' => 'textfield',
      '#default_value' => isset($settings['apachesolr_geo_facet_max_depth']) ? $settings['apachesolr_geo_facet_max_depth'] : APACHESOLR_GEO_FACET_MAX_DEPTH,
    );

 
    //TODO: option for facet item distribution

/*
  //Only relevant if range distance queries instead of default distance queries
   $form['distance_unbounded'] = array(
      '#title' => t('Unbounded item'),
      '#description' => t('Append a last facet item for matches with distance higher than the maximum value.'),
      '#type' => 'checkbox',
      '#default_value' => isset($settings['distance_unbounded']) ? $settings['distance_unbounded'] : TRUE,
    );
    */
    
  }

  /**
   * Initializes the facet's build array.
   *
   * @return array
   *   The initialized render array.
   */
  public function build() {

    // Initializes build and gets static response.
    if (!$response = apachesolr_static_response_cache($this->adapter->getSearcher())) {
      return array();
    }

    $build = array();
    $field_name = $this->facet['field'];
    $field_alias = $this->facet['field alias'];
    $active_items = $this->adapter->getActiveItems($this->facet);
    $items_by_type = $this->getItemsByType($active_items);

    // Gets total number of documents matched in search.
    $total = $response->response->numFound;


    $parent = NULL;

    if(isset($items_by_type['pt'])) {
      $key = key($items_by_type['pt']);
      $build[$key] = array('#count' => $total);
      $parent = $key;
    }
    else {
      //Build must contain something or facet widget will not be built
      $build['pt.nil:nil'] = array('#count' => $total);
    }

    if(isset($items_by_type['d'])) {

      // Gets the active date facets, starts to builds the "parent - child"
      // relationships.
      foreach ($items_by_type['d'] as $value => $item) {
        // Builds the raw facet "value", the count for selected items will be the
        // total number of rows returned in the query.
        $build[$value] = array(
          '#count' => $total,
          '#indexed_value' => $this->getIndexedValue($value), //not really needed for sorting since will only be one item in this level
        );

        // If there is a previous item, there is a parent, uses a reference so the
        // arrays are populated when they are updated.
        if (NULL !== $parent) {
          $build[$parent]['#item_children'][$value] = &$build[$value];
          $build[$value]['#item_parents'][$parent] = $parent;
        }
        // Stores the last value iterated over.
        $parent = $value;
      }
    }
     
    $settings = $this->adapter->getFacet($this->facet)->getSettings();

    if(!empty($items_by_type['d']) && !empty($settings->settings['apachesolr_geo_facet_max_depth'])) {
      if(count($items_by_type['d']) == $settings->settings['apachesolr_geo_facet_max_depth']) {
        //Set last item as excluded so that it will not get included in facet item queries (since this is the last level and we only allow one active item)
        end($items_by_type['d']);
        $active_leaf_value = key($items_by_type['d']);
        array_pop($items_by_type['d']);
        if(!empty($items_by_type['d'])) {
          end($items_by_type['d']);
          $value = key($items_by_type['d']);
          $parent = $value;
        }
        else {
          $parent = key($items_by_type['pt']);
        }
        $build[$parent]['#exclude'] = $field_alias . ':' . $active_leaf_value;
      }
    }

    $mincount = 0;

    //TODO: fix settings->settings

    if (isset($settings->settings['facet_mincount'])) {
      $mincount = $settings->settings['facet_mincount'];
    }

    $prefix = $field_name . '.';
    if (isset($response->facet_counts->facet_queries)) {
      foreach ($response->facet_counts->facet_queries as $value => $count) {
        if(strpos($value, $prefix) === 0 && $count >= $mincount) {
          $real_value = str_replace($prefix, '', $value);

          if(!isset($build[$real_value])) {
            $build[$real_value] = array();
          }
          
          $build[$real_value] = array(
            '#count' => $count,
            '#indexed_value' => $this->getIndexedValue($real_value),
          ) + $build[$real_value];

          if($parent !== NULL) {
            $build[$parent]['#item_children'][$real_value] = &$build[$real_value];
            $build[$real_value]['#item_parents'][$parent] = $parent;
          }
        }
      }
    }

    /*
    foreach ($raw_data as $value => $count) {
      if ($count) {
        $from = $value;
        $to = facetapi_isodate(strtotime($value . $gap));
        $new_value = '[' . $from . ' TO ' . $to . ']';
        $build[$new_value] = array('#count' => $count, '#active' => 0);
        if (NULL !== $parent) {
          $build[$parent]['#item_children'][$new_value] = &$build[$new_value];
          $build[$new_value]['#item_parents'][$parent] = $parent;
        }
      }
    }
     */
      
    return $build;
  }

  function getIndexedValue($value) {
    list(,$indexed_value) = explode('.', $value, 2);
    return floatval($indexed_value);
  }


}
