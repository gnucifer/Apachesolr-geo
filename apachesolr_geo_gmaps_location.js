(function ($, maps) {
Drupal.behaviors.apachesolr_geo_gmaps_location = {
  attach: function(context, settings) {
    var i;
    var facet;
    for(i in Drupal.settings.facetapi.facets) {
      facet = Drupal.settings.facetapi.facets[i];
      if(facet.widget == 'apachesolr_geo_links') {
        var $facet = $('#' + facet.id, context);
        var $form = $('.apachesolr-geo-location-selector', $facet);

        $('.form-submit', $form).hide();
        var $input = $('.location-selector', $form);

        //TODO: Perhaps option for this

        if('autocomplete_placeholder_text' in facet) {
          $input.attr('placeholder', facet.autocomplete_placeholder_text);
        }

        if($input.length) {
          var autocomplete = new maps.places.Autocomplete($input[0], {
              types : ['geocode'],
              componentRestrictions : {country : 'se'}
            }
          );
          maps.event.addListener(autocomplete, 'place_changed', function() {
              var place = autocomplete.getPlace();
              if(typeof place.geometry !== 'undefined') {
                var location = place.geometry.location;
                $form.find('.lat').val(location.lat());
                $form.find('.lng').val(location.lng());
                $form.submit();
              }
            }
          );
          $form.submit(function(e) {
              if($form.find('.lat').val() === 'nil' || $form.find('.lng').val() === 'nil') {
                e.preventDefault();
                return false; 
              }
            }
          );
        }
      }
    }

    /*
    if(typeof settings.apachesolr_geo !== 'undefined') {
      var form_suffixes = settings.apachesolr_geo.location_select_forms;
      for(i in form_suffixes) {
        //TODO: many forms on one page?? In that case we need settings for this
        var $form = $('#apachesolr-geo-location-selector-form-' + form_suffixes[i], context);
       }   
      }
    }
    /*
    var geocoder = new maps.Geocoder();
    console.log('success');
    geocoder.geocode({ 'address' : 'Styckjunkaregatan 5b', 'region' : 'SE'}, function(results, status) {
        if (status === google.maps.GeocoderStatus.OK) {
          console.dir(results);
        }
        else {
          alert(status);
        }
      }
   );
   */

  }
};
})(jQuery, google.maps);
