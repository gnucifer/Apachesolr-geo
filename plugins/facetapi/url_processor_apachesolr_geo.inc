<?php

/**
 * @file
 * The standard URL processor.
 */

/**
 * Extension of FacetapiUrlProcessor.
 */
class ApacheSolrGeoUrlProcessor extends FacetapiUrlProcessorStandard {

  /**
   * Implements FacetapiUrlProcessor::getQueryString().
   */
  public function getQueryString(array $facet, array $values, $active) {
    $qstring = $this->params;
    $active_items = $this->adapter->getActiveItems($facet);

    foreach($active_items as $key => $active_item) {
      if(!empty($active_item['exclude'])) {
        unset($values[$key]);
        unset($qstring[$this->filterKey][$active_ite['pos']]);
      }
    }

    if($active) {
      //remove all active children and self
      foreach ($values as $value) {
        if(isset($active_items[$value])) {
          unset($qstring[$this->filterKey][$active_items[$value]['pos']]);
        }
      }
    }
    else {
      //add self to field query
      //Always has just one value? What happens if heirarchy expanded? This is weird
      foreach ($values as $value) {
        $field_alias = rawurlencode($facet['field alias']);
        $qstring[$this->filterKey][] = $field_alias . ':' . $value;
      }
    }

    // Removes duplicates, resets array keys and returns query string.
    // @see http://drupal.org/node/1340528
    $qstring[$this->filterKey] = array_values(array_unique($qstring[$this->filterKey]));
    return array_filter($qstring);
  }
}
