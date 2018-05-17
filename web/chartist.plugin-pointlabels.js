/* chartist-plugin-pointlabels 0.0.6
 * Copyright © 2018 Gion Kunz
 * Free to use under the WTFPL license.
 * http://www.wtfpl.net/
 */
/* global Chartist */
(function(window, document, Chartist) {
  'use strict';

  var defaultOptions = {
    labelClass: 'ct-label',
    labelOffset: {
      x: 0,
      y: -10
    },
    textAnchor: 'middle',
    align: 'center',
    labelInterpolationFnc: Chartist.noop,
    labelFnc: undefined
  };

  var labelPositionCalculation = {
    point: function(data) {
      return {
        x: data.x,
        y: data.y
      };
    },
    bar: {
      left: function(data) {
        return {
          x: data.x1,
          y: data.y1
        };
      },
      center: function(data) {
        return {
          x: data.x1 + (data.x2 - data.x1) / 2,
          y: data.y1
        };
      },
      right: function(data) {
        return {
          x: data.x2,
          y: data.y1
        };
      }
    }
  };

  Chartist.plugins = Chartist.plugins || {};
  Chartist.plugins.ctPointLabels = function(options) {

    options = Chartist.extend({}, defaultOptions, options);

    function addLabel(position, data) {
      var value = '';
      if (options.labelFnc && typeof options.labelFnc === 'function') {
        value = options.labelFnc(data);
      } else {
        // if x and y exist concat them otherwise output only the existing value
        value = data.value.x !== undefined && data.value.y ?
          (data.value.x + ', ' + data.value.y) :
          data.value.y || data.value.x;
      }

      if(value !== '') {
        data.group.elem('text', {
          x: position.x + options.labelOffset.x,
          y: position.y + options.labelOffset.y,
          style: 'text-anchor: ' + options.textAnchor
        }, options.labelClass).text(options.labelInterpolationFnc(value));
      }
    }

    return function ctPointLabels(chart) {
      if (chart instanceof Chartist.Line || chart instanceof Chartist.Bar) {
        chart.on('draw', function(data) {
          var positonCalculator = labelPositionCalculation[data.type] && labelPositionCalculation[data.type][options.align] || labelPositionCalculation[data.type];
          if (positonCalculator) {
            addLabel(positonCalculator(data), data);
          }
        });
      }
    };
  };

}(window, document, Chartist));
