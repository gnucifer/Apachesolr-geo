<?php

/**
 * @file
 * Widgets for facets rendered as links.
 */

/**
 * Widget that renders facets as a list of clickable links.
 */
class FacetapiGeoWidgetLinks extends FacetapiWidgetLinks {

  /**
   * Overrides constructor to reset the key.
   */
  public function __construct($id, array $realm, FacetapiFacet $facet, stdClass $settings) {
    parent::__construct($id, $realm, $facet, $settings);
    $this->jsSettings['limit'] = $this->settings->settings['soft_limit'];
    $this->key = $facet['name'];
  }

  protected function processExclude(array &$build, array $exclude = array()) {
    $filter_key = $this->facet->getAdapter()->getUrlProcessor()->getFilterKey();
    foreach ($build as $value => &$item) {
      foreach($exclude as $v) {
        $key = array_search($v, $item['#query'][$filter_key]);
        if($key !== FALSE) {
          unset($item['#query'][$filter_key][$key]);
        }
      }
      if (!empty($item['#item_children'])) {
        $this->processExclude(
          $item['#item_children'],
          isset($item['#exclude']) ? array_merge($exclude, array($item['#exclude'])) : $exclude
        );
      }
    }
    return $this;
  }


  /**
   * Renders the links.
   */
  public function execute() {
    //TODO: check if field alias is correct to use here??
    $element = &$this->build[$this->facet['field alias']];

    $element_defaults = array(
      '#type' => 'container',
      '#attributes' => $this->build['#attributes'],
    );

    //TODO: fix this!
    $location_element = reset($element);
    //TODO: separate empty results from not searched yet!
    //TODO: can we redirect to remove filter if no results found?
    if(!empty($location_element['#item_children'])) {
      $this->setThemeHooks($element);
      $this->processExclude($element);
      $element = array(
        'distance' => array(
          '#theme' => 'item_list',
          '#items' => $this->buildListItems($element),
          '#prefix' => '<div class="distance-facet">',
          '#suffix' => '</div>',
        )
      ) + $element_defaults;
    }
    else {
      $element = $element_defaults;
    }

    //$elements_by_type = $this->getElementsByType($element);

    //TODO: same as above, check if field alias or name
    //TODO: refactor, geocoding source should be pluggable (somehow), separate facet??
    $element['center_point_selector'] = drupal_get_form('apachesolr_geo_location_selector_form_' . $this->facet['field alias'], $this->facet->getAdapter());
    $element['center_point_selector']['#weight'] = -1;

    if($this->settings->settings['autocomplete_placeholder_text']) {
      //TODO: does facetapi support multilingual variables? Look it up
      global $language;
      if($country = apachesolr_geo_language_country($language->language)) {
        $this->jsSettings['country_restriction'] = $country;
      }
      //How fix better i18n support, perhaps this is not good practice since translation will break if changing original etc?
      $this->jsSettings['autocomplete_placeholder_text'] = t($this->settings->settings['autocomplete_placeholder_text']);
    }

    /*
    if(isset($elements_by_type['pt'])) {
      $location_element = end($elements_by_type['pt']);
    }
    if(isset($elements_by_type['d'])) {
      $distance_elements = $elements_by_type['d'];
      $this->setThemeHooks($distance_elements);
      $element['distance'] = array(
        '#theme' => 'item_list',
        '#items' => $this->buildListItems($distance_elements),
        '#prefix' => '<div class="distance-facet">',
        '#suffix' => '</div>',
      );
    }
    */
  }

  protected function getElementsByType($element) {
    $items = array();
    foreach($element as $key => $item) {
      list($type, $value) = explode('.', $key, 2);
      $items[$type][$key] = $item;
    }
    return $items;
  }


  function settingsForm(&$form, &$form_state) {
    parent::settingsForm($form, $form_state);
    $form['widget']['widget_settings']['links'][$this->id]['autocomplete_placeholder_text'] = array(
      '#title' => t('Autocompleting location selector placeholder text'),
      '#type' => 'textfield',
      '#default_value' => isset($this->settings->settings['autocomplete_placeholder_text']) ? $this->settings->settings['autocomplete_placeholder_text'] : '',
    );
  }

 /**
   * Returns defaults for the settings this widget provides.
  */
  function getDefaultSettings() {
    return parent::getDefaultSettings() + array(
      'autocomplete_placeholder_text' => ''
    );
  }


}

//TODO: fix this
/**
 * Widget that renders facets as a list of clickable checkboxes.
 */
class FacetapiGeoWidgetCheckboxLinks extends FacetapiGeoWidgetLinks {

  /**
   * Adds JavaScript settings.
   */
  public function init() {
    parent::init();
    $this->jsSettings['makeCheckboxes'] = 1;
    drupal_add_css(drupal_get_path('module', 'facetapi') . '/facetapi.css');
  }

  /**
   * Sets the base class for checkbox facet items.
   *
   * @return array
   *   An array of classes.
   */
  public function getItemClasses() {
    return array('facetapi-checkbox');
  }
}
